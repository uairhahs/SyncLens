{
  description = "SyncLens — Qt desktop wrapper for the Resilio Sync WebUI";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      forAllSystems = nixpkgs.lib.genAttrs [ "x86_64-linux" "aarch64-linux" ];
    in
    {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          synclens = pkgs.qt6Packages.callPackage ./nix/package.nix { };
        in
        {
          inherit synclens;
          default = synclens;
        });

      overlays.default = final: _prev: {
        synclens = final.qt6Packages.callPackage ./nix/package.nix { };
      };
    };
}
