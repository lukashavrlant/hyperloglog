## Implementace HyperLogLog a LogLog algoritmů

# Instalace

Bude potřeba překladač Céčka, používal jsem gcc. Na OS X by mělo stačit

    $ gcc ./hyperloglog.c -o hyperloglog

Na Linuxu jsem použil:

    $ gcc -lm -lcrypto ./hyperloglog.c -o hyperloglog

Program má následující parametry:

    hyperloglog <input file> [<b>]

- <input file> je vstupní txt soubor. Předpokládá se, že na každém řádku je samostatné slovo, které má maximálně 255 znaků.
- <b> je integer, který určuje, kolik bitů bude použito pro vytvoření substreamů. Platí, že bude použito m substreamů, kde m = 2^b. Očekávané hodnoty jsou 4--16. Výchozí hodnota je 12. 

Příklad volání:

    $ ./hyperloglog ./1000000.txt

Program vypíše počet unikátních slov v souboru vypočítaných podle algoritmů HyperLogLog a LogLog. 

Program předpokládá, že se použije 32bitová hashová funkce. To je realizováno pomocí klasické MD5 hashovací funkce, ze které se vezme prvních 32 bitů. Při zvýšení počtu bitů je potřeba přepsat i funkci bucketIndex, protože ta natvrdo používá typ uint32_t, kvůli rychlosti. 