{
  mkShell,
  cmake,
  libssh,
  yaml-cpp,
}:
mkShell {
  packages = [
    cmake
    libssh
    yaml-cpp
  ];
}
