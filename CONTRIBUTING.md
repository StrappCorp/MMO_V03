# Workflow Git

## Branches
- `main` : branche stable / livrable
- `dev` : branche d’intégration principale
- `feature/<scope>-<sujet>` : nouvelle fonctionnalité
- `fix/<scope>-<sujet>` : correction non urgente
- `hotfix/<sujet>` : correction urgente depuis `main`

## Règles simples
1. Créer les features depuis `dev`
2. Ouvrir les PR vers `dev` pour le travail courant
3. Réserver les PR vers `main` aux releases et hotfixes
4. Garder des branches courtes et ciblées
5. Nommer les branches avec un scope clair (`combat`, `ui`, `inventory`, `network`, etc.)

## Exemples
- `feature/combat-lock-target`
- `feature/ui-main-menu`
- `fix/inventory-stack-bug`
- `hotfix/crash-startup`

## Flux recommandé
1. `git checkout dev`
2. `git pull`
3. `git checkout -b feature/<scope>-<sujet>`
4. Développement + commits propres
5. Push de la branche
6. PR vers `dev`
7. Quand `dev` est stable : PR `dev` -> `main`

## Releases
- Utiliser `main` comme référence stable
- Tagger les versions importantes si besoin (`v0.1.0`, `v0.2.0`, etc.)
