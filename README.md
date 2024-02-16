Présentation

Le but de ce TP est la mise en application des concepts liés à la synchronisation et la communication des processus à
travers l’utilisation d’outils offerts par le système d’exploitation UNIX. On se propose d’implémenter un système
d’allocation de ressources utilisant une méthode qui sera décrite par la suite :
Il s’agit de gérer l’allocation de ressources selon une méthode de réquisition définit comme suit :
- Chaque processus peut à tout moment demander et libérer des ressources à son grée,
- Quand un processus demande des ressources, si sa requête peut être satisfaite, le processus est satisfait et continu
son exécution. Dans le cas contraire, on cherche les ressources manquantes chez les processus bloqués. Si sa
requête peut être satisfaite par une partie ou la totalité des ressources détenus par ces processus, la requête du
processus est satisfaite et continu son exécution ; dans le cas contraire, il sera bloqué. Il est à remarquer que
plusieurs processus peuvent subir cette préemption.
- Quand une libération a lieu, on essaye de satisfaire en priorité les processus selon deux critères : l’ancienneté dans
l’attente et le nombre de fois qu’un processus est victime depuis son attente.

Modélisation

Les éléments qui interviennent dans le problème sont :
 Le gestionnaire de ressources implémenté par un processus Gerant cyclique qui reçoit les requêtes des
processus dans un tampon nommé TRequetes. Ce processus traite les requêtes une à une selon la méthode
indiquée ci-dessus en tenant compte de l’état actuel du système. Deux réponses sont possibles soit que la
requête est satisfaite et dans ce cas il envoie un acquittement positif à ce processus ; soit que la requête est
non satisfaite et dans ce cas il envoie une réponse négative. La réponse est envoyée dans une file de
messages commune à tous les processus nommée Freponse. D’autres parts, les libérations de ressources
sont envoyées par chaque processus i au processus Gerant dans une file nommée Fliberi propre à ce
processus.
 Les processus du système implémentés par cinq processus cycliques Calcul[i], i=1,...5, qui chacun a pour
rôle de simuler le comportement d’un processus réel ; chacun de ces processus dispose d’un fichier local
Fich[i], i=1,...5 contenant la suite d’instructions qu’il devra interpréter. Les instructions à simuler sont de
quatre types : instruction normal, instruction de demande de ressources, instruction de libération de
ressources et instruction de fin de processus. Lors du processus d’interprétation des instructions, si une
demande est rencontrée par un processus Calcul[i], i=1,...5, une requête est envoyée au processus Gérant
dans le tampon TRequetes et une attente temporaire est réalisée le temps qu’une réponse arrive. Lors de la
réception de la réponse, selon la nature de celle-ci, le processus, soit continue normalement son exécution
soit se bloque jusqu’à la réception des ressources.
 On suppose que le nombre de ressources dont dispose le système pour chaque classe est (N1, N2, N3), où Ni
est un entier positif représentant le nombre d’instances de la classe de ressources i.
Travail demandé

Il s’agit de synchroniser tous ces processus à l’aide de sémaphores dans le but de réaliser la fonction décrite ci-
dessus. Le système doit comprendre une interface (textuelle seulement, sous forme d’impressions) permettant de

suivre et de comprendre le fonctionnement des différentes entités du service objet de ce TP.

Connaissances requises sous Unix

- Création de processus
- Partage de mémoire
- Synchronisation par sémaphores
- Communication par files de messages
