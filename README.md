# Wordslide

A fun game to test your typing skills!

## Dictionary

The `dictionary.txt` file is an edited version
of the [2of5core](http://wordlist.aspell.net/12dicts-readme/#2of5core) list of words
from the [12Dicts](wordlist.aspell.net/12dicts) package version 6.0.2.
It is originally the smallest list from the package, containing about 4,700 words.
To simplify implementation and gameplay,
we have removed words from the original list
that contain hyphens, spaces, quotes, colons, or any other non-alphabetic character,
leaving around 4,500 words.
We have chosen the smallest list to improve runtime performance and overall cartridge size.
When compressed with xz, it weighs 12KB.
