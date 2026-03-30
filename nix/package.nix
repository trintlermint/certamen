{
  stdenv,
  ftxui,
  libssh,
  yaml-cpp,
}: let
  inherit (stdenv) mkDerivation;
in
  mkDerivation {
    pname = "certamen";
    version = "1.0.0";
    src = ../.;
    buildInputs = [
      ftxui
      libssh
      yaml-cpp
    ];
    buildPhase = ''
      g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 main.cpp -lyaml-cpp -o certamen
    '';
    installPhase = ''
      mkdir -p $out/bin
      cp certamen $out/bin/certamen
      chmod +x $out/bin/certamen
    '';
    meta.mainProgram = "certamen";
  }
