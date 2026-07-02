let
  system = "x86_64-linux";

  nixpie = import <nixpie>;

  inherit (nixpie.inputs.nixpkgs) lib;
  inherit (lib) attrValues;

  pkgs = import nixpie.inputs.nixpkgs {
    inherit system;
    config = {
      allowUnfree = true;
    };
    overlays = (attrValues nixpie.overlays) ++ [ nixpie.overrides.${system} ];
  };
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    cmake tbb gbenchmark fmt pcre2 libunwind
    gst_all_1.gstreamer gst_all_1.gst-plugins-base gst_all_1.gst-plugins-ugly gst_all_1.gst-plugins-bad gst_all_1.gst-plugins-good
  ];
}
