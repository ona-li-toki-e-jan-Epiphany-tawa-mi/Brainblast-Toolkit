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
#   nix build -f release.nix
# But you should probably use nix-shell + make instead.

{ nixpkgs ? builtins.fetchTarball "https://github.com/NixOS/nixpkgs/tarball/nixos-23.11"
, system  ? builtins.currentSystem
}:

let pkgs = import nixpkgs {};
    src  = builtins.fetchGit ./.;

    buildTargets = [
      "c64" "c128" "pet" "plus4"
      "cx16"
      "atari" "atarixl"
    ];

    buildForTarget = target:
      pkgs.releaseTools.nixBuild {
        name = "brainblast-toolkit-${target}";

        inherit src;

        nativeBuildInputs = with pkgs; [ cc65 ];

        makeFlags = [ "TARGET=${target}" ];

        installPhase = ''
          runHook preInstall

          cp out/* $out

          runHook postInstall
        '';
      };
in builtins.listToAttrs (
  builtins.map (target: {
    name  = target;
    value = buildForTarget target;
  }) buildTargets
)
