// dbz3 - Piezas del propio juego para las pantallas propias (marco, bocadillo, botones, fondos,
// anfitriones de los menus y caras del marcador). Se sacan de los AFS del jugador al arrancar, en
// memoria: ninguna imagen del juego va en la version publicada. Las usa style::Img.
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace dbz3::ingame::native_ui {

struct Image {
  int w = 0, h = 0;
  std::vector<uint8_t> rgba;
};

// Una vez, con la ruta del juego ya resuelta: hilo propio que lee y decodifica las piezas.
void Prefetch();
bool Ready();
// Pieza `name` ("marco", "caras/11"...): la mueve a `out` (cada una se pide una vez).
bool Take(const std::string& name, Image& out);

// [ps5 port] Una textura del juego, ya decodificada: la de XXH3 `hash` en la entrada `entry` de
// data_usi.afs de `dir` (la carpeta de la region). La usa el pack de botones de PlayStation.
bool ReadPiece(const std::filesystem::path& dir, int entry, uint64_t hash, Image& out);

}  // namespace dbz3::ingame::native_ui
