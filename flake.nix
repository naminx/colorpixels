{
  description = "Tell the ratio of color pixels / total pixels";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs";

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs { inherit system; };

      stb = pkgs.fetchFromGitHub {
        owner = "nothings";
        repo = "stb";
        rev = "f0569113c93ad095470c54bf34a17b36646bbbb5";
        sha256 = "sha256-FkTeRO/AC5itykH4uWNsE0UeQTS34VtGGoej5QJiBlg=";
      };
    in
    {
      packages.${system}.default = pkgs.stdenv.mkDerivation {
        pname = "colorpixels";
        version = "1.0";
        src = ./.;

        nativeBuildInputs = with pkgs; [
          gcc
          cli11
          gnumake
        ];
        buildInputs = with pkgs; [
          libavif
          libwebp
        ];

        preBuild = ''
          mkdir -p include
          ln -sf ${stb}/stb_image.h include/
        '';

        buildPhase = ''
          runHook preBuild
          make
          runHook postBuild
        '';

        installPhase = ''
          mkdir -p $out/bin
          cp colorpixels $out/bin/
        '';
      };

      devShells.${system}.default = pkgs.mkShell {
        packages =
          (
            with pkgs;
            [
              gcc
              libavif
              libwebp
              cli11
              clang-tools
            ]
            ++ (with python3Packages; [
              scikit-image
              tqdm
              scikit-learn
              pillow
            ])
          )
          ++ [ stb ];

        # Also create stb_image.h symlink for dev shell use:
        shellHook = ''
          mkdir -p include
          ln -sf ${stb}/stb_image.h include/

          export CPPFLAGS="$CPPFLAGS -Iinclude -I${pkgs.libavif}/include -I${pkgs.libwebp}/include -I${pkgs.cli11}/include"
          export LDFLAGS="$LDFLAGS -L${pkgs.libavif.out}/lib -L${pkgs.libwebp.out}/lib"
        '';
      };
    };
}
