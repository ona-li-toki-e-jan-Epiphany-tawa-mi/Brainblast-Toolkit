# zlib license
#
# Copyright (c) 2024 ona-li-toki-e-jan-Epiphany-tawa-mi
#
# This software is provided ‘as-is’, without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
# Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
# claim that you wrote the original software. If you use this software
# in a product, an acknowledgment in the product documentation would be
# appreciated but is not required.
#
# 2. Altered source versions must be plainly marked as such, and must not be
# misrepresented as being the original software.
#
# 3. This notice may not be removed or altered from any source
# distribution.

# release.nix for telling Hydra CI how to build the project.
#
# You can use the following command to build this/these derivation(s):
#   nix-build release.nix -A <attribute>
# But you should probably use nix-shell + make instead.

{ nixpkgs ? builtins.fetchTarball "https://github.com/NixOS/nixpkgs/tarball/nixos-23.11"
, targets ? [ "c64" "c128" "pet" "plus4" "cx16" "atari" "atarixl" ]
}:

let pkgs = import nixpkgs {};
    lib  = pkgs.lib;

    binaryTarballFor = target: pkgs.stdenv.mkDerivation rec {
        name = "brainblast-toolkit";

        src = ./.;

        nativeBuildInputs = with pkgs; [ cc65 ];

        buildPhase = ''
          runHook preBuild

          TARGETS="${target}" ./build.sh

          runHook postBuild
        '';

        doDist    = true;
        distPhase = ''
          runHook preDist

          mkdir -p $out/tarballs
          tar -cjv -f $out/tarballs/${name}-binary-dist.tar.bz2 -C ${target}/ .

          runHook postDist
        '';

        postPhases = "finalPhase";
        finalPhase = ''
          mkdir -p $out/nix-support
          for i in $out/tarballs/*; do
              echo "file binary-dist $i" >> $out/nix-support/hydra-build-products
          done
        '';
      };
in
{
  binaryTarball = lib.genAttrs targets binaryTarballFor;
}
