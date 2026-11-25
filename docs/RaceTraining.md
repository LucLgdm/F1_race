# Chrono, apprentissage des dépassements et course complète

## Utilisation

1. Ouvrir un circuit dans l'éditeur, puis lancer son entraînement habituel.
2. Dans le panneau **Entrainement**, choisir **Course - depassement**.
   Si un champion chrono du circuit est déjà chargé, il sert de point de départ
   lorsqu'aucun champion course compatible n'existe encore.
3. Pour choisir explicitement un ancien champion, renseigner son fichier `.f1`,
   puis cliquer **Course depuis un champion chrono**.
4. Retirer la pause. Le mode accéléré conserve le pas physique de 1/120 s.
5. Pour observer un champion course enregistré, sélectionner son fichier
   `_race.f1`, puis **Voir le champion**, et retirer la pause.

Le mode chrono conserve les réseaux 8 → 16 → 16 → 3 et les voitures indépendantes.
Le mode course utilise 23 → 32 → 32 → 3. Il affiche un seul candidat et sa voiture
lente à la fois : les vingt candidats sont évalués dans des mondes distincts.
Le mode **Course complete** réunit les voitures dans un seul peloton.

Le premier exercice demande une route d'au moins 8 m de large. Une génération
comprend quatre situations de dépassement et un essai sans trafic, identiques
pour tous les candidats. Le véhicule lent suit la ligne de piste avec un décalage
latéral fixe, à 8, 10 ou 12 m/s. Distance initiale, décalage et point de départ varient.
Un dépassement est réussi avec au moins 6 m d'avance maintenus pendant 1 s,
sans contact. L'essai solo demande jusqu'à 120 m de progression.
Les essais s'arrêtent également après 30 s ou 4 s sans progrès suffisant.

Le classement privilégie le nombre de réussites, puis la somme des progressions
normalisées des essais, puis le temps total. Lorsque les cinq essais sont réussis,
la progression totale vaut cinq : le temps départage alors les réseaux.
Le meilleur réseau est conservé sans mutation à la génération suivante.
Le bilan affiché correspond au meilleur candidat de la dernière génération achevée.
L'apprentissage utilise la sélection et les mutations, pas la rétropropagation MSE.
Aucun nombre de générations ne garantit la réussite.

## Course complète sur le circuit

Après l'apprentissage des dépassements, choisir **Course complete** dans le menu.
Le mode reprend le champion de dépassement compatible du circuit ou, en l'absence
de sauvegarde, le réseau de travail conservé à l'indice zéro des exercices (l'élite
de la dernière génération achevée). Pour choisir un fichier précis, renseigner son
chemin `_race.f1` puis cliquer **Lancer une course avec ce reseau** ou
**Charger ce reseau pour la course**. Un champion chrono seul ne suffit pas : il
ne possède pas les entrées de perception du trafic.

Le départ comprend **8 voitures et 3 tours** par défaut. Les champs **Voitures**
(2 à 20) et **Tours** (1 à 20) règlent la prochaine course. **Nouvelle course**
reconstruit la grille et remet le chronomètre à zéro, en pause. Retirer la pause
pour partir. Les voitures partagent initialement des copies identiques de la
politique sélectionnée, sans mutation de leurs paramètres pendant la course.
L'API accepte aussi des politiques différentes par participant.

La grille est décalée sur deux colonnes entièrement derrière la ligne de départ.
Le premier passage sur la ligne arme le chronométrage des tours ; il ne constitue
pas un tour accompli. Le classement utilise la position cumulée depuis la ligne,
puis classe les arrivants selon leur temps d'arrivée interpolé au passage de la
ligne. Il affiche les numéros, couleurs, tours accomplis et motifs d'abandon.
La caméra suit la voiture active en tête ; une voiture arrêtée ne la bloque pas.

Les observations décrivent les trois voisines les plus proches parmi les voitures
présentes sur la piste, en excluant la voiture observatrice. Chaque pas de 1/120 s
comprend les phases suivantes : instantanés, observations, commandes, prédictions
indépendantes, collisions communes, validation des déplacements et classement.
Après un déplacement rejeté, les autres trajectoires sont vérifiées à nouveau
pour empêcher une troisième voiture d'entrer dans la pose réoccupée.

Un contact entraîne l'abandon des voitures concernées à leur dernière pose sans
chevauchement. Elles deviennent grises, restent des obstacles et sont toujours
perçues par les autres. Une sortie de piste ou 10 secondes sans progrès suffisant
entraîne également un abandon. Les voitures arrivées quittent la piste active ;
le vainqueur est affiché dans la vue finale. La course s'arrête quand plus aucune
voiture n'est active, ou à une limite de temps adaptée à la longueur et au nombre
de tours (`max(120, longueur × tours / 5 + 60)` secondes). L'affichage se remet
alors en pause et quitte l'accélération pour montrer le résultat.

Ce mode exécute les réseaux entraînés : il n'ajoute pas d'apprentissage dans le
peloton et ne sauvegarde pas ses temps dans les records chrono ou dans le score
des exercices. Réussir les exercices ne garantit pas une course propre face à
plusieurs voitures. Les fichiers `.track` et `.f1` restent inchangés pour cette étape.
Les simulations indépendantes et les espaces de travail par voiture préparent
une exécution parallèle ; la résolution des collisions reste une barrière commune.

## Observations et réseau

Les huit valeurs existantes sont inchangées : cinq rayons vers les bords, vitesse,
sinus et cosinus de l'angle à la piste. Trois emplacements supplémentaires décrivent
les voisines les plus proches à 40 m : présence, position longitudinale et latérale,
vitesse relative longitudinale et latérale. Les positions sont normalisées par 40 m,
les vitesses relatives par 100 m/s (deux véhicules peuvent se rapprocher à 50 m/s
chacun). Les repères tournent avec le véhicule observateur. Un emplacement absent
contient cinq zéros. Le véhicule observateur doit être exclu de la liste des voisines.

Le transfert d'un champion chrono recopie les neurones existants dans les couches
élargies et met tous les paramètres supplémentaires à zéro. Les commandes initiales
sont conservées même lorsque le trafic est présent. Les mutations peuvent ensuite
faire intervenir les nouvelles entrées et les neurones ajoutés. Ce transfert demande
exactement deux couches cachées de 32 neurones au maximum et trois sorties.

## Collisions et futur rendu 3D

`VehicleGeometry.hpp` centralise les dimensions du dessin actuel et du gabarit physique.
Le rectangle de collision orienté mesure actuellement 4 × 3 m : il comprend les roues,
y compris leur débattement. Une décoration différente ne demande aucun changement
aux capteurs. Si les dimensions physiques changent, mettre à jour le gabarit dans ce
fichier. Le rectangle demeure une approximation des volumes, avec des coins vides.

Le test continu des positions et orientations entre deux pas détecte les traversées
rapides. La translation utilise un SAT balayé exact ; la rotation utilise une avance
conservative avec une tolérance de contact de 0,1 mm et un arrêt conservateur si la
résolution atteint sa limite. Il n'y a pas de marge de sécurité artificielle en mètres.
En cas de contact, les deux véhicules restent au dernier pas sans contact et l'essai
échoue. Les chocs, rebonds et responsabilités ne sont pas simulés.

Les collisions avec la piste conservent le cercle historique de rayon 2,3 m et les
vérifications `canTravel`, afin de préserver les conditions des anciens records chrono.
Les collisions entre véhicules utilisent le nouveau rectangle. Un futur rendu 3D
sur une même surface peut réutiliser ces calculs au sol. Des niveaux superposés
nécessiteront une information de hauteur ou de surface.

## Fichiers

Les fichiers `.track` de géométrie ne changent pas. Les champions chrono restent
au format `F1_CHAMPION 2`; la lecture du format historique 1 est conservée.
Les dimensions et les poids de chaque couche sont déjà écrits dans les `.f1` :
une couche de 32 neurones se sauvegarde sans modifier la géométrie.

Les champions course utilisent `F1_CHAMPION 3`, `MODE Race`, `OBSERVATIONS 2 40 50`,
le gabarit physique et `EVALUATION 1`. La lecture refuse une version d'observation,
un gabarit ou une version d'exercice incompatibles. Les sorties conservent la version 1.
Par compatibilité de structure, `LAP_TIME` contient en mode course le **temps total
des cinq exercices réussis**, pas un temps au tour. Le panneau l'affiche comme record
sur la suite. La sauvegarde automatique n'a lieu qu'après cinq réussites.

Exemple : `Le_Mans.f1` pour le chrono, `Le_Mans_race.f1` pour les dépassements.
Un fichier existant associé à une autre géométrie ou illisible est conservé ; une
variante numérotée est choisie. Les anciens champions ne sont pas réécrits lors du
transfert. Les sauvegardes restent des champions, pas des checkpoints de population.

## Préparation CPU / GPU

Aucun multithreading ni kernel GPU n'est activé dans cette version.

- Les observations sont des tableaux fixes de 23 flottants, les instantanés des
  structures simples sans pointeurs propriétaires.
- Toutes les décisions sont calculées avant les déplacements. Les scores et
  réinitialisations sont traités après cette phase, à une barrière explicite.
- Chaque candidat écrit uniquement dans son propre épisode et lit le circuit partagé.
  L'aléatoire intervient uniquement pendant la reproduction, sur le thread de contrôle.
- `Network::forwardInto` utilise des espaces de travail propres à l'appelant, sans
  allocation à chaque inférence. `parameters()` fournit les poids et biais dans
  un tableau contigu par couche, et `topology()` fournit les dimensions.
- Les étapes de calcul sont séparées d'OpenGL et d'ImGui. L'affichage reste dans
  l'application. Les emplois CPU et un backend GPU pourront conserver le même contrat.

Un backend GPU demandera encore de transférer la géométrie et les paramètres vers
la mémoire du GPU et de porter l'inférence et la simulation en kernels. Les classes
C++ actuelles contenant des vecteurs ne peuvent pas être copiées telles quelles sur
le GPU. Le choix devra être guidé par des mesures de temps et la taille de population.

## Vérification

Les tests `[ml][race]` couvrent les observations, collisions continues et orientées,
transfert de réseau, formats de sauvegarde, lecture des champions existants,
réussite d'un dépassement contrôlé et d'un essai solo, rejet d'un contact, classement,
évaluation identique de deux politiques et conservation de l'élite.
Ils vérifient le mécanisme d'apprentissage et les exercices, pas la convergence
stochastique d'une population aléatoire.

Les tests `[ml][race_session]` vérifient la grille sans chevauchement, la perception
du peloton, un dépassement avec changement de leader, les contacts simultanés et
les réactions en chaîne après rejet d'un déplacement. Ils vérifient aussi deux
tours complets sur un circuit arrondi, l'ordre d'arrivée, les abandons, les limites
de course et l'absence de modification des réseaux pendant la course.
