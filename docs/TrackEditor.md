# Éditeur de circuits C++

L'application s'ouvre dans l'éditeur ImGui. Aucune nouvelle bibliothèque n'est nécessaire : la vue
de l'éditeur utilise les listes de dessin ImGui, rendues par le backend OpenGL existant. Le jeu
utilise son renderer OpenGL habituel.

## Utilisation

1. Dans **Ouvrir**, choisir un fichier du dossier `circuits/`, ou partir de **Nouveau** / **Rectangle**.
   `esses.track` fournit une boucle avec des virages à droite et à gauche. Un ancien champion `.f1`
   peut également fournir le dessin de départ.
2. Ajouter des **Droites** et des **Virages**. Une nouvelle pièce se place après la sélection.
3. Glisser une pièce pour la déplacer, sa poignée carrée pour changer sa longueur / son rayon,
   et sa poignée ronde pour tourner. Les champs numériques permettent un réglage précis.
4. Régler la largeur commune, le sens des virages et l'ordre des pièces. **Raccorder au précédent**
   aligne exactement deux extrémités. **Déplacer aussi la suite** conserve les raccords suivants
   et leurs changements de direction. **Fermer par une droite** relie la dernière pièce à la première.
5. Positionner le départ sur la piste avec le bouton dédié ou avec sa distance depuis la première pièce.
6. Donner un nom puis **Enregistrer .track**. Les brouillons ouverts restent sauvegardables.
7. Une fois **Boucle jouable** affiché, choisir **Conduire** ou **Entraîner les voitures**.

La molette zoome autour du curseur. Le clic droit ou central déplace la vue. **Alt** désactive
l'accrochage pendant une manipulation. **Annuler** / **Rétablir** conservent les dernières opérations.
Les commandes en bas du panneau sont accessibles en faisant défiler celui-ci sur une petite fenêtre.

En conduite, utiliser les flèches, ZQSD ou WASD ; **R** replace la voiture. Les rayons peuvent être
affichés, ainsi que le numéro de pièce, la distance locale et le décalage latéral. **Échap** revient à
l'éditeur ; la fermeture de la fenêtre quitte le programme. L'entraînement démarre en pause.

La case **Vue de tout le circuit**, disponible dans les panneaux de conduite et d'entraînement,
bascule entre le suivi d'une voiture et le cadrage automatique de l'ensemble du tracé. En conduite,
le suivi cible ta voiture ; en entraînement, il cible la voiture ayant la meilleure progression.
La vue globale suit les dimensions réelles de la piste et le format de la fenêtre, avec une marge.
Le panneau ImGui occupe une zone réservée à gauche pour éviter de masquer une partie du circuit.
La caméra de l'éditeur reste indépendante de ce choix.

## Fichiers et champions

Lancer le programme depuis la racine du dépôt, comme auparavant : `./build/F1_race.exe` sous MSYS2.
Les chemins `shaders/` et `circuits/` sont relatifs au répertoire de lancement.

- `.track` contient uniquement le circuit : dessin, largeur et départ. Les brouillons incomplets
  sont acceptés. La sauvegarde explicite remplace le fichier du même nom.
- `.f1` conserve le fonctionnement des champions : circuit, réseau de neurones, configuration
  d'observation et meilleur temps. Les fichiers `F1_CHAMPION 1` / `RECTANGLE` existants restent lisibles.
  Les nouvelles sauvegardes utilisent `F1_CHAMPION 2` et embarquent la géométrie complète.
- L'entraînement réutilise le champion du même nom si son tracé correspond exactement au dessin.
  Si ce nom appartient à un autre circuit ou à un fichier illisible, un suffixe numérique préserve
  le fichier existant. Les meilleurs temps continuent à être sauvegardés automatiquement.
- Le panneau d'entraînement permet toujours de revoir un champion ou de reprendre son entraînement.
  Charger un tracé `.f1` dans l'éditeur importe son dessin ; le réseau est chargé par le panneau
  d'entraînement ou réutilisé lors du démarrage d'un entraînement correspondant.

Les écritures passent par un fichier temporaire fermé et vérifié avant remplacement, avec restauration
de l'original si le remplacement échoue. Les données numériques utilisent la locale classique et
la précision nécessaire au rechargement des `float`.

## Format `.track`

```text
F1_TRACK 1
ROAD_WIDTH 12
START_DISTANCE 20
SEGMENTS 2
STRAIGHT 0 0 0 80
CURVE 80 0 0 30 -1
END_TRACK
```

Les coordonnées et distances sont en mètres ; les angles sont en radians, dans le sens antihoraire.
`STRAIGHT x y heading length` décrit un rectangle orienté depuis son extrémité de départ.
`CURVE x y heading radius turn` décrit un quart de cercle tangent à cette direction : `turn=1`
tourne à gauche, `turn=-1` à droite. Le rayon concerne la ligne centrale. Cet exemple est un brouillon
ouvert ; il faut une boucle pour conduire ou entraîner les agents.

## Géométrie et simulation

`Track` construit un ruban triangulé et ses bords à partir des segments ordonnés. Les raccords
partagent leurs extrémités et ne créent pas de murs internes. Les angles droits des anciens rectangles
utilisent des raccords en onglet. Les courbes sont discrétisées pour le rendu et les collisions ; les
capteurs utilisent exactement les mêmes bords. Les calculs de longueur et de projection sur les arcs
utilisent leur géométrie analytique.

`TrackLocation` regroupe la pièce, la distance sur cette pièce, la distance bouclée depuis le départ,
la tangente et le décalage latéral. La simulation limite la projection à un voisinage de la coordonnée
précédente pour éviter un saut vers une autre portion proche du circuit. Cette coordonnée alimente
les tours et le score des agents ; sa tangente fournit l'erreur de direction au réseau. Les huit entrées
du réseau restent compatibles avec les anciens champions.

La voiture conserve son enveloppe circulaire de rayon 2,3 m. Un contrôle du volume balayé complète
le contrôle de sa position finale, afin d'empêcher une traversée des bords. Les grands pas de temps
sont subdivisés à 120 Hz. L'orientation initiale et la remise à zéro suivent la tangente du circuit.

L'édition porte sur un brouillon indépendant du circuit actif. Le changement de circuit reconstruit
la simulation, les compteurs et la population. Les références vers le circuit ne deviennent donc pas
invalides pendant une édition.

## Limites

Les virages sont des quarts de cercle et la largeur est commune. Les tracés ouverts, les demi-tours
serrés et les croisements sont signalés et ne peuvent pas démarrer une course. Les croisements avec
ponts et les intersections routières ne sont pas pris en charge. La limite est de 256 pièces et
4 096 quadrilatères après discrétisation. La conduite conserve le modèle physique simple du projet.

## Vérification

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Les tests couvrent les segments orientés, les deux sens de virage, les raccords sans mur, la projection
locale, les collisions balayées, la compatibilité des champions et les sauvegardes. Les tests ImGui
créent un contexte sans fenêtre et exercent les commandes et les manipulations du plan ; ils ne
requièrent pas de GPU ou de serveur graphique pour être exécutés en CI.
