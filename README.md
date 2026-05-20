# Projekt_PSiO
Proponowany podział ról
Osoba Adam Zawitowski — Gameplay / Mechaniki gry

Odpowiada za logikę gry i zachowanie świata.

Zadania
ruch gracza
chodzenie
skok
grawitacja
sprint / dash (opcjonalnie)
kolizje
z podłożem
ze ścianami
z przeciwnikami
z przedmiotami
system przeciwników
patrolowanie
wykrywanie kolizji
obrażenia / śmierć
system punktów i życia
respawn/checkpointy
logika wygranej/przegranej
fizyka platformówki
balans gameplayu
Dodatkowo
przygotowanie klas:
Player
Enemy
Physics
CollisionSystem

Osoba Wiktor Zalewski — Silnik gry / Grafika / Mapy:

Odpowiada za rendering, assety i strukturę gry.

Zadania
renderowanie obiektów SFML
kamera / scrolling mapy
tilemapa
ładowanie poziomów
animacje sprite’ów
UI/HUD
licznik punktów
życie
menu
ekran game over
audio
muzyka
efekty dźwiękowe
asset management
tekstury
fonty
dźwięki
optymalizacja renderingu
Dodatkowo
przygotowanie klas:
Renderer
TileMap
Animation
AudioManager
UIManager
Zadania wspólne

Te rzeczy warto robić razem:

Projekt architektury

struktura folderów
naming convention
ustalenie stylu kodu
podział klas
UML (jeśli wymagany)

Game design

wygląd poziomów
mechaniki
przeciwnicy
power-upy
poziom trudności

Testowanie

bug fixing
playtesty
debugowanie

Dokumentacja

README
instrukcja uruchamiania
opis klas
komentarze

Plan projektu
Etap 1 — Setup projektu

repo GitHub
SFML
build system
okno gry
podstawowy loop gry

Etap 2 — Core gameplay
ruch gracza
platformy
kolizje
kamera

Etap 3 — Rozgrywka
przeciwnicy
punkty
życie
animacje

Etap 4 — Content
poziomy
menu
audio
efekty

Etap 5 — Finalizacja
bug fixing
balans
dokumentacja
refactor
Technologie, które warto dodać
Build system

Minimum:

ruch gracza
skok
kolizje
przeciwnik
jeden poziom
punkty lub życie
ekran końca gry
scrolling kamery
podstawowe animacje

Dodatki, jeśli starczy czasu
save system
editor poziomów
power-upy
parallax background
AI przeciwników
particle effects
multiplayer lokalny

Przykładowy weekly plan
Tydzień 1

setup projektu
SFML
GitHub
player movement

Tydzień 2
kolizje
tilemapa
kamera

Tydzień 3
przeciwnicy
UI
audio

Tydzień 4
poziomy
testy
poprawki
dokumentacja

