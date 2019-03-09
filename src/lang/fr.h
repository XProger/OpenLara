#ifndef H_LANG_FR
#define H_LANG_FR

// Thanks: Zellphie

const char *STR_FR[] = { ""
// help
    , "Chargement..."
    , "Appuyez sur H pour afficher l'aide"
    , helpText
    , "%s@@@"
      "ENNEMIS TU)ES %d@@"
      "OBJETS TROUVES %d@@"
      "SECRETS %d / %d@@"
      "TEMPS %s"
    , "Sauvegarde..."
    , "Sauvegarde achev)ee!"
    , "SAUVER ERREUR!"
    , "OUI"
    , "NON"
    , "Arret"
    , "Marche"
    , ")Ecran Divis)e"
    , "VR"
    , "Bas"
    , "Moyen"
    , "Haut"
    , STR_LANGUAGES
    , "Appliquer"
    , "Gamepad 1"
    , "Gamepad 2"
    , "Gamepad 3"
    , "Gamepad 4"
    , "Not Ready"
    , "Joueur 1"
    , "Joueur 2"
    , "Appuyez sur une touche"
    , "%s - Choisir"
    , "%s - Retour"
// inventory pages
    , "OPTIONS"
    , "INVENTAIRE"
    , "OBJETS"
// save game page
    , "Sauver le jeu?"
    , "Charger Partie"
// inventory option
    , "Jeu"
    , "Carte"
    , "Boussole"
    , "Statistiques"
    , "Manoir de Lara"
    , "Niveau des D)etails"
    , "Sons"
    , "Controles"
    , "Gamma"
// passport menu
    , "Charger"
    , "Commencer"
    , "Rejouer Niveau"
    , "Sortie vers titre"
    , "Quitter"
    , "Choisir Niveau"
// detail options
    , "Niveau de D)etail"
    , "Filtration"
    , ")Eclairage"
    , "Ombres"
    , "Eau"
    , "VSync"
    , "St)er)eo"
    , "Objets Simples"
// sound options
    , "R)egler Volume"
    , "R)everb)eration"
    , "Sous-titres"
    , "Langue"
// controls options
    , "R)egler Controles"
    , "Clavier"
    , "Gamepad"
    , "Vibration"
    , "Reciblage"
    , "Multi-vis)ee"
    // controls
    , "Gauche", "Droite", "Courir", "Arri$ere", "Sauter", "Marcher", "Action", "D)egainer", "Regarder", "Duck", "Dash", "Roulade", "Inventaire", "Start"
    // keys
    , "NONE", "LEFT", "RIGHT", "UP", "DOWN", "SPACE", "TAB", "ENTER", "ESCAPE", "SHIFT", "CTRL", "ALT"
    , "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
    , "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M"
    , "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
    // gamepad
    , "NONE", "A", "B", "X", "Y", "L BUMPER", "R BUMPER", "SELECT", "START", "L STICK", "R STICK", "L TRIGGER", "R TRIGGER", "D-LEFT", "D-RIGHT", "D-UP", "D-DOWN"
// inventory items
    , "Inconnu"
    , "Explosif"
    , "Pistolets"
    , "Fusil $a pompe"
    , "Magnums"
    , "Uzis"
    , "Balles de pistolet"
    , "Cartouches de fusil"
    , "Balles de magnum"
    , "Balles d'uzi"
    , "Mini m)edikit"
    , "Grand m)edikit"
    , "Barre en plomb"
    , "Scion"
// keys
    , "Cl)e"
    , "Cl)e d'argent"
    , "Cl)e rouill)ee"
    , "Cl)e en or"
    , "Cl)e de saphir"
    , "Cl)e Neptune"
    , "Cl)e d'Atlas"
    , "Cl)e Damocl$es"
    , "Cl)e de Thor"
    , "Cl)e Ornement)ee"
// puzzles
    , "Puzzle"
    , "Idole d'or"
    , "Lingot d'or"
    , "Engrenage"
    , "Fusible"
    , "Ankh"
    , "Oeil d'Horus"
    , "Sceau d'Anubis"
    , "Scarab)ee"
    , "Cl)e pyramide"
// TR1 subtitles
    /* CAFE */ ,
    ""
    /* LIFT */ ,
    ""
    /* CANYON */ ,
    ""
    /* PRISON */ ,
    ""
    /* 22 */ ,
    "[04000]Vous revoil$a?"
    "[05500]Comme vous, pour la grand r)e-ouverture je pr)esume."
    "[09000]L')evolution s'enlise, la s)el)ection naturelle est plus lente que jamais"
    "[13500]cette chair bien fra(iche r)eveillera les haines ancestrales et cruelles, "
    "[17500]nous donnant force et conviction"
    "[20500]et m(eme de nouvelles races."
    "[22500]Une )evolution gr(ace aux st)ero~ides, en fait."
    "[24500]Un bon coup de fouet. Ces idiots de Qualopec et Tihocan sont des ignares."
    "[29500]Le cataclysme de l'Atlantide a engendr)e une race de mauviettes,"
    "[33500]les ramenant au stade primitif de la simple survie."
    "[37000]|ca n'aurait jamais d(u arriver."
    "[39000]Moi, j'arrive."
    "[40000]L')eclosion commence dans 15 secondes."
    "[43000]Trop tard pour un avortemen!"
    "[45000]Pas sans le coeur de l'op)eration!"
    "[47000]Noooon!"
    "[49500]10..."
    "[54000]5..."
    "[55000]4...3...2..."
    "[60000]1..."
    /* 23 */ ,
    "[00001]Bon, vous avez mon attention maintenant,"
    "[02000]mais je ne sais pas si j'ai la v(otre."
    "[05000]Allez!"
    "[06000]Je vais vous en faire bouffer de l'avoine, moi."
    "[09000]Mais oui..."
    "[10000]Vous, et ce satan)e morceau de Scion."
    "[13000]Si vous y tenez tant que |ca, je vais vous l'coller dans le.."
    "[17000]Attendez. Vous parlez de l'artefact, l$a?"
    "[19500]Un peu, mon n'veu! J'vais vous le..."
    "[22000]Oh l$a! Du calme!"
    "[24000]Un morceau, vous disiez. O$u est le reste?"
    "[26500]Miss Natla a mis Pierre DuPont sur l'affaire."
    "[29500]A quel endroit?"
    "[30500]Ha! Vous n'(etes pas assez rapide pour lui!"
    "[33500]Donc je perds du temps $a discuter avec vous ici?"
    "[37000]Moi je ne sais pas du tout o$u il court,@avec ses petites pattes de pied tendre."
    "[42000]Allez demander $a Miss Natla."
    "[46000]"
    "[50500]Merci! Je vais..."
    /* 24 */ , ""
    /* 25 */ ,
    "[03500]Ci-g(it Tihocan..."
    "[05500]... un des deux souverains justes de l'Atlantide..."
    "[10000]qui m(eme apr$es le malheur de ce continent..."
    "[13000]... tent$erent de r)egner encore sur ces terres d)esol)ees."
    "[19000]Mort sans un h)eritier, son savoir n'a point d'h)eritage."
    "[25500]Veillez sur nous, Tihocan."
    /* 26 */ , "Bienvenue chez moi! je vais vous faire visiter."
    /* 27 */ , "Utilisez les touches fl)ech)ees pour@aller dans le salon de musique."
    /* 28 */ , "OK. On va bouger un peu.@Appuyez sur la touche de saut."
    /* 29 */ , "Recommencez en appuyant sur une touche fl)ech)ee@et je sauterai dans cette direction."
    /* 30 */ , "Oh! Voici le grand hall!@Excusez le d)esordre, mais je veux tout envoyer au garde-meuble@mais les d)em)enageurs ne sont toujours pas arriv)es."
    /* 31 */ , "Courez vers une caisse et appuyez@en m(eme temps sur la touche fl)ech)ee haut et la touche d'action@je sauterai dessus."
    /* 32 */ , "C')etait autrefois la salle de bal,@mais je l'ai transform)ee en salle de gym personnelle.@C'est chouette, non?@Allez, un peu d'exercice."
    /* 33 */ , "Je ne cours pas sans arr(et dans ce jeu en fait.@Quand je dois (etre prudente, je peux aussi marcher.@Appuyez sur la touche de marche et avancez@jusqu'$a la ligne blanche."
    /* 34 */ , "Si vous appuyez encore sur la touche de marche,@je ne tomberai pas, m(eme si vous insistez.@Allez-y, essayez!"
    /* 35 */ , "Si vous voulez examiner les environs,@appuyez sur la touche qui vous permet de voir.@Puis sur une touche de direction."
    /* 36 */ , "Si je ne peux pas sauter assez loin, je peux toujours me rattraper@de justesse pour ne pas tomber. Avancez vers la ligne blanche@jusqu'$a ce que je m'arr(ete de moi-m(eme,@ensuite appuyez sur la touche de saut et tout de suite apr$es@sur la touche fl)ech)ee haut, et maintenez la touche d'action enfonc)ee pendant que je saute."
    /* 37 */ , "Appuyez sur la touche fl)ech)ee haut pour me faire grimper."
    /* 38 */ , "Si je prends de l')elan, je peux sauter |ca sans probl$eme!"
    /* 39 */ , "Avancez jusqu'$a la ligne blanche, jusqu'$a ce que je m'arr(ete,@et appuyez sur la touche de marche puis sur la touche fl)ech)ee bas pour que je prenne de l')elan.@Appuyez ensuite sur la touche fl)ech)ee haut et sur la touche de saut tout de suite apr$es.@Mais en fait, je ne sauterai qu'au dernier moment, je ne suis pas folle!"
    /* 40 */ , "L$a, c'est plus difficile.@Nous allons refaire exactement le m(eme saut, mais@vous maintiendrez la touche d'action enfonc)ee@pour que je me rattrape en catastrophe@Quand je serai en l'air."
    /* 41 */ , "Bien jou)e!"
    /* 42 */ , "Nous allons essayer de sauter ici.@Appuyez sur la touche fl)ech)ee haut puis sur la touche d'action sans la rel(acher."
    /* 43 */ , "Je n'ai pas la place de grimper i|ci, mais si vous appuyez sur la touche@fl)ech)ee droite je longerai la corniche pour trouver un passage@ et vous n'aurez plus qu'$a appuyer sur la touche fl)ech)ee haut $a ce moment l$a."
    /* 44 */ , "Super!@Si je me retrouve trop haut je peux me laisser@glisser doucement, pour )eviter de me blesser en sautant."
    /* 45 */ , "Appuyez sur la touche fl)ech)ee bas pour que je me laisse tomber@ensuite si vous gardez la touche d'action enfonc)ee@je me rattraperai dans ma chute."
    /* 46 */ , "Ensuite laissez aller."
    /* 47 */ , "Et maintenant un bon bain!"
    /* 48 */ , "La touche de saut et les touches de direction@me permettent d')evoluer sous l'eau."
    /* 49 */ , "Oh! Ah! De l'air!@Utilisez les touches fl)ech)ees avant, gauche et droite pour me faire nager en surface.@Appuyez sur la touche de saut pour me faire plonger et nager sous l'eau.@Pour me faire sortir, dirigez-vous vers le bord et appuyez sur la touche d'action."
    /* 50 */ , "Super. Bon je ferais mieux d'enlever ces v(etements mouill)es."
    /* 51 */ , "Un petit sourire!"
    /* 52 */ , "Ca n'a rien de personel."
    /* 53 */ , "J'ai encore mal au cr(ane $a cause de toi. Et j'entends des petites voix dans ma t(ete.@Elles me disent de te tuer!"
    /* 54 */ , "Vous n'allez pas m')eliminer pas avec ma race aussi facilement Lara!"
    /* 55 */ , "Un peu tard pour la remise des prix non?@Ah... L'essentiel, c'est de participer? mais me voici maintenant devant un horrible dilemme!@C'est bien dommage, mais c'est la vie."
    /* 56 */ , "C'EST SUR MOI QUE TU TIRES?! C'EST SUR MOI QUE TU TIRES?! @BAH OUI Y'A PERSONNE! @C'EST SUR MOI QUE TU TIRES!!"
// TR1 levels
    , "Manoir de Lara"
    , "Cavernes"
    , "Cit)ee de Vilcabamba"
    , "Vall)ee Perdue"
    , "Tombe de Qualopec"
    , "Monument St. Francis"
    , "Colosseum"
    , "Palais de Midas"
    , "La Citerne"
    , "Tombe de Tihocan"
    , "Cit)ee de Khamoon"
    , "Ob)elisque de Khamoon"
    , "Sanctuaire du Scion"
    , "Mines de Natla"
    , "Atlantide"
    , "La Grande Pyramide"
    , "Retour en Egypte"
    , "Temple du Chat"
    , "La Forteresse Atlantique"
    , "La Ruche"
// TR2 levels
    , "Lara's Home"
    , "The Great Wall"
    , "Venice"
    , "Bartoli's Hideout"
    , "Opera House"
    , "Offshore Rig"
    , "Diving Area"
    , "40 Fathoms"
    , "Wreck of the Maria Doria"
    , "Living Quarters"
    , "The Deck"
    , "Tibetan Foothills"
    , "Barkhang Monastery"
    , "Catacombs of the Talion"
    , "Ice Palace"
    , "Temple of Xian"
    , "Floating Islands"
    , "The Dragon's Lair"
    , "Home Sweet Home"
// TR3 levels
    , "Lara's House"
    , "Jungle"
    , "Temple Ruins"
    , "The River Ganges"
    , "Caves Of Kaliya"
    , "Coastal Village"
    , "Crash Site"
    , "Madubu Gorge"
    , "Temple Of Puna"
    , "Thames Wharf"
    , "Aldwych"
    , "Lud's Gate"
    , "City"
    , "Nevada Desert"
    , "High Security Compound"
    , "Area 51"
    , "Antarctica"
    , "RX-Tech Mines"
    , "Lost City Of Tinnos"
    , "Meteorite Cavern"
    , "All Hallows"
};

#endif
