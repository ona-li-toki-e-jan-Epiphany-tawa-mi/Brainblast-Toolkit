{ pkgs ? import <nixpkgs> {} }:

(pkgs.buildFHSEnv {
  name = "cc65-build-environment";
  targetPkgs = pkgs: with pkgs; [
    cc65
    gnumake

    vice
    x16-emulator x16-rom
  ];
}).env
