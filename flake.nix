# This file is part of BASICfuck.
#
# Copyright (c) 2024-2025 ona-li-toki-e-jan-Epiphany-tawa-mi
#
# BASICfuck is free software: you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# BASICfuck is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# BASICfuck. If not, see <https://www.gnu.org/licenses/>.

{
  description = "BASICfuck development environment";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { nixpkgs, ... }:
    let
      inherit (nixpkgs.lib) genAttrs systems;

      forSystems = f:
        genAttrs systems.flakeExposed
        (system: f { pkgs = import nixpkgs { inherit system; }; });
    in {
      devShells = forSystems ({ pkgs, ... }:
        let inherit (pkgs) buildFHSEnv;
        in {
          default = (buildFHSEnv {
            name = "brainblast-toolkit-build-environment";

            targetPkgs = pkgs:
              with pkgs; [
                cc65
                astyle

                vice
                x16-emulator
                x16-rom
                atari800
              ];
          }).env;
        });
    };
}
