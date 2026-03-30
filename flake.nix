{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };
  outputs = inputs @ {flake-parts, ...}:
    flake-parts.lib.mkFlake {inherit inputs;} {
      systems = ["x86_64-linux" "aarch64-linux" "aarch64-darwin" "x86_64-darwin"];
      perSystem = {pkgs, ...}: rec {
        devShells.default = pkgs.callPackage ./nix/shell.nix {};
        packages.certamen = pkgs.callPackage ./nix/package.nix {};
        packages.default = packages.certamen;
      };
    };
}
