# Publish-Subscribe
Publish-Subscribe limited message distribution system up to N capacity.


**3 pliki:**
- `main.c` - plik testowy
- `list.c` - wlasciwy kod
- `list.h` - będzie szablon od sobanca

- Struktura będzie bombardowana zapytaniami, ma się nie wywalić i działać optymalnie pod względem pamięci.
- Musi implementować interfejs określony przez sobanca.

**Dla List:**
- Wyciągnięcie elementu z pustej listy usypia wątek do momentu pojawienia się jakiegos elementu.
- Lista dynamiczny sposób implementacji - linked lista za pomocą wskaźników lub wszystkie elementy w zwykłej tablicy wskaźników i w miarę potrzeby realokujemy pamięć.
- Addmsg  blokująca jeżeli nje ma miejsca, podobnie getmsg.

**Sprawozdanie:**
- W sprawozdaniu opisać istotne informacje o strukturach danych szczególnie dla 2 projektu
- im szybciej się wyślę tym lepszy feedback się dostanie *(jak się za późno wyślę to tylko info o błędzie, jak szybciej to wskazówki naprowadzające)*
- na ostatnich zajęciach wszyscy powinni mieć już oceny
- na zajęciach w styczniu pol zajęć tipy od sobanca, druga połowa czas ma dyskusje i pytania
- będzie weryfikacja autorstwa projektu także trzeba wiedzieć co dana rzecz robi
- Skała puntów na ocenę końcowa łącznie jest 40 pkt do uzyskania, 20 test, max 20 projekt
- ostateczna skalę sobaniec potem poda, na pewno od 20 zaliczenie ale może prog obniżyć
