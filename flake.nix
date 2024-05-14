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

{
  description = "Brainblast-Toolkit development environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.11";

  outputs = { self, nixpkgs }:
    let
      lib = nixpkgs.lib;

      forSystems = f: lib.genAttrs lib.systems.flakeExposed (system: f {
        pkgs = import nixpkgs { inherit system; };
      });
    in
      {
        devShells = forSystems ({ pkgs }: {
          default = with pkgs; (buildFHSEnv {
            name = "brainblast-toolkit-build-environment";
            targetPkgs = pkgs: with pkgs; [
              cc65

              vice
              x16-emulator x16-rom
              atari800
            ];
          }).env;
        });
      };
  }
