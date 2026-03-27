#include "serve.hpp"

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pty.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>

static volatile sig_atomic_t g_running = 1;

static void sighandler(int) { g_running = 0; }

static std::string timestamp()
{
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}

static void log_info(const std::string& msg)
{
    std::cout << "[" << timestamp() << "] " << msg << "\n" << std::flush;
}

static void log_err(const std::string& msg)
{
    std::cerr << "[" << timestamp() << "] ERRO: " << msg << "\n" << std::flush;
}

static bool ensure_host_key(const std::string& path)
{
    if (std::filesystem::exists(path)) return true;

    log_info("Generating host key: " + path);
    ssh_key key = nullptr;
    int rc = ssh_pki_generate(SSH_KEYTYPE_RSA, 2048, &key);
    if (rc != SSH_OK || key == nullptr)
    {
        log_err("Failed to generate RSA key.");
        return false;
    }
    rc = ssh_pki_export_privkey_file(key, nullptr, nullptr, nullptr, path.c_str());
    ssh_key_free(key);
    if (rc != SSH_OK)
    {
        log_err("Failed to write key to " + path);
        return false;
    }
    std::filesystem::permissions(path,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace);
    return true;
}

// data inside ssh channel and PTY master using poll().
static void bridge_io(ssh_channel channel, int master_fd, ssh_session session)
{
    char buf[8192];
    int ssh_fd = ssh_get_fd(session);
    if (ssh_fd < 0) return;

    // master_fd : non-blocking so reads don't stall
    int flags = fcntl(master_fd, F_GETFL, 0);
    if (flags >= 0) fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);

    while (ssh_channel_is_open(channel) && !ssh_channel_is_eof(channel))
    {
        struct pollfd fds[2];
        fds[0].fd = ssh_fd;
        fds[0].events = POLLIN;
        fds[1].fd = master_fd;
        fds[1].events = POLLIN;

        int ret = poll(fds, 2, 200);
        if (ret < 0)
        {
            if (errno == EINTR) continue;
            break;
        }

        if (fds[0].revents & (POLLIN | POLLHUP | POLLERR))
        {
            // PUT THE FRIES IN THE BAGGGGGGG
            ssh_execute_message_callbacks(session);

            while (true)
            {
                int n = ssh_channel_read_nonblocking(channel, buf, sizeof(buf), 0);
                if (n > 0)
                {
                    ssize_t off = 0;
                    while (off < n)
                    {
                        ssize_t w = write(master_fd, buf + off, static_cast<size_t>(n - off));
                        if (w < 0)
                        {
                            if (errno == EAGAIN || errno == EINTR) continue;
                            goto done;
                        }
                        off += w;
                    }
                }
                else if (n == SSH_ERROR)
                {
                    goto done;
                }
                else
                {
                    break; // n == 0, no more data right now
                }
            }

            if (ssh_channel_is_eof(channel)) break;
        }

        //send tos ssh channel when has content 
        if (fds[1].revents & POLLIN)
        {
            while (true)
            {
                ssize_t n = read(master_fd, buf, sizeof(buf));
                if (n > 0)
                {
                    int w = ssh_channel_write(channel, buf, static_cast<uint32_t>(n));
                    if (w == SSH_ERROR) goto done;
                }
                else if (n < 0 && (errno == EAGAIN || errno == EINTR))
                {
                    break;
                }
                else
                {
                    break;
                }
            }
        }

        // PTY hung up => child exit
        if (fds[1].revents & (POLLHUP | POLLERR))
        {
            // gc
            while (true)
            {
                ssize_t n = read(master_fd, buf, sizeof(buf));
                if (n > 0)
                    ssh_channel_write(channel, buf, static_cast<uint32_t>(n));
                else
                    break;
            }
            break;
        }
    }

done:
    return;
}
static int message_callback(ssh_session, ssh_message msg, void* userdata)
{
    int master_fd = *static_cast<int*>(userdata);

    if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL &&
        ssh_message_subtype(msg) == SSH_CHANNEL_REQUEST_WINDOW_CHANGE)
    {
        int cols = ssh_message_channel_request_pty_width(msg);
        int rows = ssh_message_channel_request_pty_height(msg);
        struct winsize ws{};
        ws.ws_col = static_cast<unsigned short>(cols);
        ws.ws_row = static_cast<unsigned short>(rows);
        ioctl(master_fd, TIOCSWINSZ, &ws);
        ssh_message_channel_request_reply_success(msg);
        return 0; // handled
    }

    return 1; // not handle => default data
}

static void handle_client(ssh_session session,
                          const std::string& password,
                          const std::vector<std::string>& quiz_files,
                          const std::string& self_path)
{
    log_info("Starting key exchange...");
    if (ssh_handle_key_exchange(session) != SSH_OK)
    {
        log_err(std::string("Key exchange failed.. big sad.. go ask dev: ") + ssh_get_error(session));
        return;
    }
    log_info("Key exchange complete.");
    bool authenticated = false;
    std::string client_user;
    for (int attempts = 0; attempts < 20 && !authenticated; ++attempts)
    {
        ssh_message msg = ssh_message_get(session);
        if (!msg) break;

        if (ssh_message_type(msg) == SSH_REQUEST_AUTH)
        {
            int subtype = ssh_message_subtype(msg);
            const char* user = ssh_message_auth_user(msg);
            if (user) client_user = user;

            if (subtype == SSH_AUTH_METHOD_PASSWORD)
            {
                const char* pw = ssh_message_auth_password(msg);
                if (password.empty() || (pw && password == pw))
                {
                    ssh_message_auth_reply_success(msg, 0);
                    authenticated = true;
                    log_info("Authenticated: " + client_user + " (password)");
                }
                else
                {
                    log_info("Authentication rejected: wrong password from " + client_user);
                    ssh_message_auth_set_methods(msg, SSH_AUTH_METHOD_PASSWORD);
                    ssh_message_reply_default(msg);
                }
            }
            else if (subtype == SSH_AUTH_METHOD_NONE)
            {
                if (password.empty())
                {
                    ssh_message_auth_reply_success(msg, 0);
                    authenticated = true;
                    log_info("Authenticated: " + client_user + " (none)");
                }
                else
                {
                    ssh_message_auth_set_methods(msg, SSH_AUTH_METHOD_PASSWORD);
                    ssh_message_reply_default(msg);
                }
            }
            else
            {
                // advertise password
                if (!password.empty())
                    ssh_message_auth_set_methods(msg, SSH_AUTH_METHOD_PASSWORD);
                else
                    ssh_message_auth_set_methods(msg, SSH_AUTH_METHOD_NONE);
                ssh_message_reply_default(msg);
            }
        }
        else
        {
            ssh_message_reply_default(msg);
        }
        ssh_message_free(msg);
    }

    if (!authenticated)
    {
        log_err("Authentication failed: " + client_user);
        return;
    }

    // wait for channel to open
    ssh_channel channel = nullptr;
    for (int i = 0; i < 30 && !channel; ++i)
    {
        ssh_message msg = ssh_message_get(session);
        if (!msg) break;

        if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL_OPEN &&
            ssh_message_subtype(msg) == SSH_CHANNEL_SESSION)
        {
            channel = ssh_message_channel_request_open_reply_accept(msg);
            log_info("Channel opened for " + client_user);
        }
        else
        {
            ssh_message_reply_default(msg);
        }
        ssh_message_free(msg);
    }

    if (!channel)
    {
        log_err("No channel from " + client_user);
        return;
    }

    // stall for PTY && shell requests => get user terminal dims
    bool got_pty = false;
    bool got_shell = false;
    int term_cols = 80, term_rows = 24;
    std::string term_type = "xterm-256color";

    for (int i = 0; i < 30 && !(got_pty && got_shell); ++i)
    {
        ssh_message msg = ssh_message_get(session);
        if (!msg) break;

        if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL)
        {
            int sub = ssh_message_subtype(msg);
            if (sub == SSH_CHANNEL_REQUEST_PTY)
            {
                term_cols = ssh_message_channel_request_pty_width(msg);
                term_rows = ssh_message_channel_request_pty_height(msg);
                const char* t = ssh_message_channel_request_pty_term(msg);
                if (t && t[0]) term_type = t;
                ssh_message_channel_request_reply_success(msg);
                got_pty = true;
                log_info("PTY: " + std::to_string(term_cols) + "x" +
                    std::to_string(term_rows) + " term=" + term_type +
                    " user=" + client_user);
            }
            else if (sub == SSH_CHANNEL_REQUEST_SHELL)
            {
                ssh_message_channel_request_reply_success(msg);
                got_shell = true;
            }
            else if (sub == SSH_CHANNEL_REQUEST_WINDOW_CHANGE)
            {
                term_cols = ssh_message_channel_request_pty_width(msg);
                term_rows = ssh_message_channel_request_pty_height(msg);
                ssh_message_channel_request_reply_success(msg);
            }
            else
            {
                ssh_message_reply_default(msg);
            }
        }
        else
        {
            ssh_message_reply_default(msg);
        }
        ssh_message_free(msg);
    }

    if (!got_shell)
    {
        log_err("No shell request from " + client_user);
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return;
    }

    // fork a PTY child with the right terminal size (thank you rtfm)
    struct winsize ws{};
    ws.ws_col = static_cast<unsigned short>(term_cols);
    ws.ws_row = static_cast<unsigned short>(term_rows);
    // /tmp
    char metrics_tmpl[] = "/tmp/certamen_metrics_XXXXXX";
    int metrics_fd = mkstemp(metrics_tmpl);
    if (metrics_fd >= 0) close(metrics_fd);
    std::string metrics_path = metrics_tmpl;

    int master_fd = -1;
    pid_t child = forkpty(&master_fd, nullptr, nullptr, &ws);

    if (child < 0)
    {
        log_err("forkpty failed: " + std::string(strerror(errno)));
        std::filesystem::remove(metrics_path);
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        return;
    }

    if (child == 0)
    {
        setenv("TERM", term_type.c_str(), 1);

        std::vector<std::string> args;
        args.push_back(self_path);
        args.push_back("--session");
        args.push_back("--metrics");
        args.push_back(metrics_path);
        for (const auto& f : quiz_files) args.push_back(f);

        std::vector<char*> argv;
        for (auto& a : args) argv.push_back(a.data());
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        perror("execvp");
        _exit(1);
    }

    log_info("Session started: user=" + client_user + " pid=" + std::to_string(child));
    ssh_set_message_callback(session, message_callback, &master_fd);
    bridge_io(channel, master_fd, session);
    close(master_fd);

    int status = 0;
    waitpid(child, &status, 0);

    int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    log_info("Session ended: user=" + client_user + " pid=" + std::to_string(child) +
             " exit=" + std::to_string(exit_code));

    std::ifstream mf(metrics_path);
    if (mf.is_open())
    {
        std::string line;
        while (std::getline(mf, line))
            log_info("METRICS [" + client_user + "]: " + line);
        mf.close();
        std::filesystem::remove(metrics_path);
    }

    if (ssh_channel_is_open(channel))
    {
        ssh_channel_send_eof(channel);
        ssh_channel_close(channel);
    }
    ssh_channel_free(channel);
    log_info("Disconnected: " + client_user);
}

int serve_main(int port,
               const std::string& password,
               const std::vector<std::string>& quiz_files,
               const std::string& host_key_path,
               int max_clients)
{
    if (!ensure_host_key(host_key_path))
        return 1;

    std::string self_path = std::filesystem::read_symlink("/proc/self/exe").string();

    ssh_bind sshbind = ssh_bind_new();
    if (!sshbind)
    {
        log_err("ssh_bind_new failed.");
        return 1;
    }

    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_BINDPORT, &port);
    ssh_bind_options_set(sshbind, SSH_BIND_OPTIONS_RSAKEY, host_key_path.c_str());

    if (ssh_bind_listen(sshbind) < 0)
    {
        log_err(std::string("Listen failed: ") + ssh_get_error(sshbind));
        ssh_bind_free(sshbind);
        return 1;
    }

    log_info("Certamen SSH server listening on port " + std::to_string(port));
    log_info("Connect: ssh -p " + std::to_string(port) + " <name>@<host>");
    if (password.empty())
        log_info("Auth: open (no password)");
    else
        log_info("Auth: password required");
    log_info("Max clients: " + std::to_string(max_clients));
    for (const auto& f : quiz_files)
        log_info("  Quiz: " + f);

    // Use sigaction without SA_RESTART
    // => accept() ret EINTR 
    // on signal
    struct sigaction sa{};
    sa.sa_handler = sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    signal(SIGCHLD, SIG_IGN);

    int active_clients = 0;

    while (g_running)
    {
        ssh_session session = ssh_new();
        if (!session) continue;

        long timeout_s = 2;
        ssh_options_set(session, SSH_OPTIONS_TIMEOUT, &timeout_s);

        int rc = ssh_bind_accept(sshbind, session);
        if (rc != SSH_OK)
        {
            ssh_free(session);
            continue;
        }

        // death to the masses!
        while (waitpid(-1, nullptr, WNOHANG) > 0)
            --active_clients;

        if (max_clients > 0 && active_clients >= max_clients)
        {
            log_info("Rejecting connection: max clients (" +
                     std::to_string(max_clients) + ") reached.");
            ssh_disconnect(session);
            ssh_free(session);
            continue;
        }

        log_info("Connection accepted (" + std::to_string(active_clients + 1) + "/" +
                 std::to_string(max_clients) + ")");

        pid_t pid = fork();
        if (pid == 0)
        {
            ssh_bind_free(sshbind);
            handle_client(session, password, quiz_files, self_path);
            ssh_disconnect(session);
            ssh_free(session);
            _exit(0);
        }
        else if (pid < 0)
        {
            log_err("fork failed: " + std::string(strerror(errno)));
            ssh_disconnect(session);
            ssh_free(session);
        }
        else
        {
            ++active_clients;
            // P must not disconnect; child owns the session fd
            ssh_free(session);
        }
    }

    log_info("Shutting down.");
    ssh_bind_free(sshbind);
    return 0;
}
