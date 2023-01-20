# Projekt-z-Przetwarzania-Rozproszonego-Stowarzyszenie-umar-ych-poet-w
1. Opis algorytmu <br />
Odgórnie podana jest liczba poetów, którzy są oddzielnymi procesami. W przypadku
braku dostępnych kółek, to proces decyduje się na kandydaturę na lidera. Korzystając z
algorytmu zegaru lamporta do sekcji krytycznej zostają proces ma szansę zostać liderem.
Jeśli w międzyczasie nie dołączył do innej grupy oraz liczba wszystkich współistniejących
kółek nie osiągnęła maksimum, a proces wszedł do sekcji krytycznej, to zostaje on liderem.
Następnie wysyła wszystkim pozostałym poetom informację o nowo utworzonym kółku.
Poeci, jeśli nadal szukają grupy, to odsyłają mu wiadomość czy chcą dołączyć do danego
kółka, czy też nie - także jest to wybierane dla każdego poety losowo. Lider przyjmuje
odpowiedzi od poetów i jeśli liczba członków przekroczy 3 (jeden, który przynosi alkohol,
jeden, który przynosi zagrychę, + jeden, który nie przynosi nic), to włączany jest timer do
momentu zamknięcia grupy. W czasie jego działania nowi poeci nadal mogą do niej
dołączać. Po przekroczeniu czasu zgłaszający, który wciąż chciałby się dostać do grupy
otrzymuje od lidera odpowiedź negatywną i dalej szuka grupy. Do wszystkich poetów
wysyłana jest wiadomość, że ta grupa została już zamknięta i znika z listy dostępnych grup.
Następuje przejście do podziału obowiązków zakupowych przed rozpoczęciem libacji.
<br />
Każdy z poetów posiada listę odległości czasowych od momentu, kiedy przynosił
daną rzecz lub też był sępem. Przykładowo, poeta, który przyniósł alkohol 2 libacje temu,
zagrychę 3 libacje temu, a był sępem na ostatniej będzie posiadał następującą listę - [2, 3,
0]. Każdy porównuje z każdym swoją listę i na tej podstawie typowane są osoby, które od
najdłuższego czasu nic nie przyniosły. Odpowiednie czasy zerowane są u osób, które coś
przynoszą i u osób, które będą sępami. Wybierają pokój z listy nieskończonych pokoi i
przeprowadzają libację, po której odpoczywają i wysyłają wiadomość do losowego
wolontariusza o pokoju do sprzątnięcia.
<br />
Zanim jeszcze komunikaty będą przyjmowane przez wolontariuszy na początku
pracy programu, to tworzona jest lista kolejności zgodnie, z którą będą pracować. Każdy
wolontariusz posiada wiedzę o pokojach, które posiada inny wolontariusz - wymiana
informacji następuje ciągłym broadcastem, jeśli wolontariusz dostaje informacje o nowym
pokoju, to wysyła swoją zaktualizowaną listę innym. Na podstawie listy kolejności
przydzielane są do danego wolontariusza pokoje, przykładowo jeden z wolontariuszy
posiada następującą liczbę pokoi: [2,3,4]. Jest pierwszy na liście wolontariuszy, tj. teraz on
powinien wybrać pokój, zatem wybierze 2 i poinformuje o tym pozostałych wolontariuszy,
którzy skreślą 2 z listy dostępnych dla siebie pokoi.
<br />
<br />
2. Założenia.
- Kanały między procesami są FIFO i niezawodne.<br />
- Procesy nie ulegają awarii.<br />
- Za moment zakończenia uznajemy moment, kiedy jedno kółko kończy libacje i
przestaje istnieć.<br />
- Pomijamy czas przetwarzania lokalnego.<br />
- Jednostkowy czas przesyłania wiadomości.<br />
- broadcast wysyłany jest do wszystkich z wyłączeniem siebie samego, n-1.<br />
<br />
<br />
3. Złożoność czasowa.<br />
Najpierw procesy ubiegają się o sekcje krytyczną, w jej skład wchodzą 3 tury - REQUEST,
REPLY, RELEASE. Następnie, gdy proces zostanie już liderem, to broadcastuje informacje o
swoim powstaniu do wszystkich (+1 tura). Procesy odsyłają mu swoje decyzje o dołączeniu
do grupy (+1 tura). Lider odsyła akceptacje bądź odrzucenie do poetów, którzy się do niego
zgłosili (+1 tura). Po zamknięciu grupy ponownie wysyła o tym informację do zebranych w
grupie poetów (+1 tura). Ustalamy, który poeta przynosi alkohol i zagrychę - wobec tego
musimy rozesłać swoją tablicę czasową, zawierającą informacje o ostatnio przyniesionych
produktach i byciu sępem, po wszystkich procesach w grupie (+1 tura). Dalsza część
wybierania ustalana jest wewnętrznie i po zakończeniu wyboru lider informuje o rozpoczęciu
libacji (+1 tura) pozostałe procesy. Po zakończeniu libacji zostaje rozesłana do wszystkich
poetów w grupie przez lidera informacja o usunięciu grupy (+1 tura).<br />
Złożoność czasowa:10
<br />
<br />
4. Złożoność komunikacyjna<br />
Ubieganie się o sekcję krytyczną posiada złożoność komunikacyjną o wielkości 3(n - 1) Lider
informuje wszystkie procesy o utworzeniu nowej grupy (n -1). Procesy decydują się o
dołączeniu lub nie i odsyłają wiadomość do lidera (n - 1). Lider odsyła swoją akceptację lub
odrzucenie (n - 1). Przechodzimy do wysłania informacji o zamknięciu grupy (pwg - 1, pwg -
poeci w grupie, niewiadoma liczba, bo mogli nie chcieć dołączyć albo nie zdążyli dołączyć
przed zamknięciem grupy). Wysłanie do siebie informacji o produktach (pwg - 1). Później
rozpoczęcie libacji (pwg - 1) i rozwiązanie grupy (n - 1, bo informujemy o tym wszystkie
procesy, by zaktualizowały ilość współistniejących na raz grup w swoich zmiennych).<br />
Złożoność komunikacyjna: 7(n - 1) + 3(pwg - 1)
