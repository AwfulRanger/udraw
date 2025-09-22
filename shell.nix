{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
	nativeBuildInputs = [
		pkgs.cmake
		pkgs.bluez
	];
}
