{
  cmake,
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
      cmake
      ftxui
      libssh
      yaml-cpp
    ];
    configurePhase = ''
      cmake -B build -DCMAKE_BUILD_TYPE=Release
    '';
    buildPhase = ''
      cmake --build build
    '';
    installPhase = ''
      mkdir -p $out/bin
      cp ./build/bin/certamen $out/bin/certamen
      chmod +x $out/bin/certamen
    '';
    meta.mainProgram = "certamen";
  }
