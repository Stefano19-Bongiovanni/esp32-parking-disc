## Mobile app plan

L'app deve avere un aspetto moderno e pensato per essere vista in verticale su uno schermo di telefono.

1. Aggiornare `mobile_app/src/style.css` per definire variabili CSS riutilizzabili:
   - `--color-primary: #6C3A5C`
   - variabili per sfondo, testo, bordo, pulsante, ombra
   - stili base mobile e layout verticale centrato

2. Creare un componente header comune in `mobile_app/src/components/HeaderBar.vue`.
   - deve accettare props: `title` e `showBackButton`.
   - quando `showBackButton` è vero mostra un'icona MDI `chevron-left` a sinistra.
   - il pulsante back usa la navigazione router per tornare indietro.
   - tutte le viste lo includono in cima.
   - nella Home viene mostrato solo il titolo "Home" senza back button.

3. Creare un componente pulsante riutilizzabile in `mobile_app/src/components/PrimaryButton.vue`.
   - gestisce label, click event e stile uniforme.
   - permette usare lo stesso aspetto su Home e altrove.

4. Creare le viste in `mobile_app/src/views`:
   - `Home.vue` su `/` con titolo "Home" e due pulsanti affiancati in basso: "Immagine" e "Disco orario".
   - `ParkingDisk.vue` su `/parking-disk` con header comune e un contenuto placeholder.
   - `Drawing.vue` su `/draw` con header comune e un contenuto placeholder.

5. Configurare `mobile_app/src/router.js` con le rotte:
   - `/` -> Home
   - `/parking-disk` -> ParkingDisk
   - `/draw` -> Drawing

6. Mantenere `mobile_app/src/App.vue` come shell che rende `<RouterView />`.

Verifica:

- la home deve caricarsi su `/` e mostrare due pulsanti in basso affiancati.
- il colore primario usato deve essere `#6C3A5C`.
- la vista `/draw` e `/parking-disk` mostrano l'header con il titolo e la freccia indietro.
- il pulsante indietro torna alla pagina precedente.
