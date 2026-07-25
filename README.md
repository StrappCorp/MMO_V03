# MMO_V03

Projet Unreal Engine 5.7 pour le client MMORPG.

## Stack
- Unreal Engine 5.7
- Git
- Git LFS

## Branches
- `main` : branche stable
- `dev` : branche de travail principale

## Workflow recommandé
1. Travailler depuis `dev`
2. Créer des branches de feature si nécessaire
3. Revenir sur `dev` pour l'intégration
4. Fusionner vers `main` pour les versions stables

## Notes
- Les assets lourds passent par Git LFS
- Éviter de versionner les dossiers générés localement (`Intermediate`, `Saved`, `DerivedDataCache`)
