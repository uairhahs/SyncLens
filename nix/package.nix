{
  lib,
  stdenv,
  cmake,
  ninja,
  pkg-config,
  wrapQtAppsHook,
  qtbase,
  qtwebengine,
  qtsvg,
  libsecret,
}:

stdenv.mkDerivation {
  pname = "synclens";
  version = "0.2.0";

  src = lib.cleanSource ../.;

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    wrapQtAppsHook
  ];

  buildInputs = [
    qtbase
    qtwebengine
    qtsvg
    libsecret
  ];

  cmakeFlags = [ "-DCMAKE_BUILD_TYPE=Release" ];

  meta = {
    description = "Unofficial Qt desktop wrapper for the Resilio Sync WebUI";
    homepage = "https://github.com/uairhahs/SyncLens";
    license = lib.licenses.mit;
    platforms = lib.platforms.linux;
    mainProgram = "synclens";
  };
}
