{
  description = "an advanced incremental SAT solver";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    cadical = {
      url = "github:meelgroup/cadical/add_dynamic_lib";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    cadiback = {
      url = "github:meelgroup/cadiback/synthesis";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };
  outputs =
    {
      self,
      nixpkgs,
      cadical,
      cadiback,
    }:
    let
      inherit (nixpkgs) lib;
      systems = lib.intersectLists lib.systems.flakeExposed lib.platforms.linux;
      forAllSystems = lib.genAttrs systems;
      nixpkgsFor = forAllSystems (system: nixpkgs.legacyPackages.${system});
      fs = lib.fileset;

      cryptominisat-package =
        {
          stdenv,
          fetchFromGitHub,
          cmake,
          cadiback,
          cadical,
          pkg-config,
          gmp,
          zlib,
        }:
        stdenv.mkDerivation {
          name = "cryptominisat";
          src = fs.toSource {
            root = ./.;
            fileset = fs.unions [
              ./src
              ./CMakeLists.txt
              ./cmake
              ./scripts
              ./cryptominisat5Config.cmake.in
            ];
          };
          patchPhase = ''
            substituteInPlace src/backbone.cpp \
              --replace-fail "../cadiback/cadiback.h" "${cadiback}/include/cadiback.h"
          '';

          nativeBuildInputs = [
            cmake
            pkg-config
          ];
          buildInputs = [
            cadiback
            cadical
            gmp
            zlib
          ];
        };
    in
    {
      packages = forAllSystems (
        system:
        let
          cryptominisat = nixpkgsFor.${system}.callPackage cryptominisat-package {
            cadical = cadical.packages.${system}.cadical;
            cadiback = cadiback.packages.${system}.cadiback;
          };
        in
        {
          inherit cryptominisat;
          cryptominisat5 = cryptominisat;
          default = cryptominisat;
        }
      );
    };
}
