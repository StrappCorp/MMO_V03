# Checklist Unreal Engine / Git LFS

## Avant commit
- [ ] Vérifier que seuls les vrais changements de travail sont présents dans `git status`
- [ ] Ne pas ajouter de dossiers générés : `Binaries/`, `DerivedDataCache/`, `Intermediate/`, `Saved/`
- [ ] Vérifier que les fichiers de solution / IDE locaux ne partent pas au repo (`.vs/`, `.vscode/`, `.idea/`, `*.sln`)
- [ ] Confirmer que les assets Unreal `.uasset` et `.umap` restent suivis par Git LFS

## Avant push
- [ ] Ouvrir le projet et vérifier qu’il charge correctement
- [ ] Régénérer / compiler seulement si le changement le nécessite
- [ ] Vérifier qu’aucun asset cassé ou redirector inutile n’est resté dans le projet
- [ ] Relire les fichiers de config modifiés avant envoi

## Bonnes pratiques LFS
- [ ] Garder `.gitattributes` versionné
- [ ] Éviter de renommer / déplacer massivement des assets sans raison
- [ ] Faire les gros mouvements d’assets dans des PR dédiées
- [ ] Prévenir l’équipe avant un push lourd en assets

## Rappel repo actuel
Ce dépôt suit déjà les fichiers suivants via Git LFS :
- `*.uasset`
- `*.umap`
