# SecureFilesGuard

**SecureFilesGuard** est une application permettant de protéger des fichiers importants, comme des tokens de comptes. Elle a été développée en utilisant Claude, Mistral AI, et ChatGPT. Bien que l'application soit fonctionnelle, certains bugs peuvent se produire, et il arrive que l'explorateur Windows se bloque occasionnellement.

Voici le lien de la conversation avec claude: https://claude.ai/share/330748bc-b283-4d0e-8349-00d5ae176ba7

## Fonctionnalités

- **Protection des fichiers sensibles** : Protégez vos fichiers et dossiers importants.
- **Surveillance des accès** : L'application surveille les accès aux fichiers protégés et vous avertit lorsqu'une application tente d'y accéder.
- **Contrôle total** : Vous pouvez autoriser ou bloquer les accès à des fichiers ou dossiers spécifiques via une interface simple.

## Prérequis

- Un système Windows (Windows 10 ou supérieur recommandé).
- Droits administratifs pour installer les pilotes et exécuter l'application.

## Installation

### Étape 1 : Télécharger le fichier

1. Rendez-vous sur la [section des releases](https://github.com/Dotmazy/SecureFilesGuard/releases).
2. Téléchargez le fichier ZIP de la dernière version.

### Étape 2 : Installer le logiciel

1. Décompressez le fichier ZIP téléchargé.
2. Exécutez `install.bat` en tant qu'administrateur. Il vous sera demandé de redémarrer votre ordinateur.
3. Après le redémarrage, vous verrez un texte dans le coin inférieur droit de votre écran (près de votre barre des tâches) disant que vous êtes en `Mode test`. Ce texte ne peut pas être supprimée, car elle est liée à un pilote que je ne peux pas signer sans un certificat payant.

### Étape 3 : Lancer le pilote

1. Exécutez `start.bat` en tant qu'administrateur pour démarrer le pilote.
2. Une fois le pilote lancé, vous pouvez maintenant exécuter `SecureFilesGuard.exe` en tant qu'administrateur pour démarrer l'application.

## Utilisation

1. **Ajouter des fichiers ou des dossiers** : Cliquez sur le bouton pour ajouter des éléments à la liste des "éléments sécurisés".
2. **Connecter le pilote** : Cliquez sur le bouton "Connect" pour établir une connexion avec le pilote. Si vous recevez un message d'erreur, assurez-vous que le pilote a bien été installé et démarré.
3. **Protection des fichiers** : Une fois le pilote connecté, tous les fichiers et dossiers listés seront protégés. Chaque fois qu'un processus essaiera d'y accéder, une fenêtre contextuelle apparaîtra pour vous avertir. Vous aurez le choix d'autoriser ou de refuser l'accès.
   - **Autoriser l'accès** : Si vous autorisez l'accès à un processus, il n'y aura plus de demande la prochaine fois que ce processus essaiera d'accéder au fichier ou dossier.
   - **Refuser l'accès** : Si vous refusez l'accès, le processus sera immédiatement tué et l'application continuera à vous alerter à chaque tentative d'accès du même processus.

## Remarques

- L'application utilise un pilote non signé, ce qui entraîne l'apparition de l'icône dans la barre des tâches. Malheureusement, je ne peux pas signer ce pilote à cause des frais associés à cette opération.
- Certains utilisateurs ont rapporté des crashs de l'explorateur Windows lorsqu'une action de protection est effectuée. Ce problème est en cours d'investigation.

## Contribution

Je ne suis pas un expert dans ce domaine, donc si vous avez des compétences ou des idées pour améliorer ce projet, n'hésitez pas à contribuer ! Vous pouvez soumettre des *pull requests* pour corriger des bugs, ajouter des fonctionnalités, ou améliorer la performance du projet.

Si vous trouvez un bug ou une anomalie, vous pouvez également ouvrir une **issue**. Bien que je ne sois pas toujours en mesure de résoudre rapidement les problèmes, je ferai de mon mieux pour les examiner et essayer de les corriger. Vos retours et contributions sont très appréciés.

## Rapport de problèmes

Si vous rencontrez un problème avec l'application (par exemple, un crash ou un comportement inattendu), merci de le signaler via une *issue* sur GitHub. Même si je ne pourrai pas toujours résoudre immédiatement les problèmes, je vais faire mon possible pour les analyser et fournir des solutions ou des suggestions.

## Licence

Ce projet est sous la licence **GNU General Public License** (GPL). Vous êtes libre de l'utiliser, de le copier, de le modifier et de le redistribuer, tant que vous respectez les termes de cette licence.

Voir le fichier [LICENSE](LICENSE) pour plus de détails.

---

*Merci d'utiliser SecureFilesGuard. Votre sécurité est notre priorité !*
