# This file is part of Brainblast-Toolkit.
#
# Copyright (c) 2024 ona-li-toki-e-jan-Epiphany-tawa-mi
#
# Brainblast-Toolkit is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# Brainblast-Toolkit is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# Brainblast-Toolkit. If not, see <https://www.gnu.org/licenses/>.

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
        makeFlags         = [ "TARGET=${target}" ];

        # We don't need to do anything here but nix whines about not having an
        # install target in the makefile without this.
        installPhase = ''
          runHook preInstall
          runHook postInstall
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
