#!/bin/bash

if [[ $# != 1 ]]; then
    echo "usa $0 unix_path"
    exit 1
fi

rm -f valgrind_out
/usr/bin/valgrind --leak-check=full ./chatty -f DATA/chatty.conf1 >& ./valgrind_out &
pid=$!

# aspetto un po' per far partire valgrind
sleep 5

# registro un po' di nickname
./client -l $1 -c pippo &
./client -l $1 -c pluto &
./client -l $1 -c minni &
./client -l $1 -c topolino &
./client -l $1 -c paperino &
./client -l $1 -c qui &
./client -l $1 -c quo &
./client -l $1 -c qua &
wait

# minni deve ricevere 8 messaggi prima di terminare
./client -l $1 -k minni -R 8 &
pid=$!

# aspetto un po' per essere sicuro che il client sia partito
sleep 1

# primo messaggio
./client -l $1 -k topolino -S "ciao da topolino":minni
if [[ $? != 0 ]]; then
    exit 1
fi
# secondo e terzo
./client -l $1 -k paperino -S "ciao da paperino":minni -S "ciao ciao!!!":minni
if [[ $? != 0 ]]; then
    exit 1
fi
# quarto
./client -l $1 -k qui -S "ciao a tutti": 
if [[ $? != 0 ]]; then
    exit 1
fi
# quinto e sesto
./client -l $1 -k quo -S "ciao a tutti":  -S "ciao da quo":minni
if [[ $? != 0 ]]; then
    exit 1
fi
# settimo ed ottavo
./client -l $1 -k qua -S "ciao a tutti":  -S "ciao da qua":minni -p
if [[ $? != 0 ]]; then
    exit 1
fi

wait $pid
if [[ $? != 0 ]]; then
    echo "ESCO8"
    exit 1
fi

# messaggio di errore che mi aspetto
OP_NICK_ALREADY=26

# provo a ri-registrare pippo
./client -l $1 -c pippo
e=$?
if [[ $((256-e)) != $OP_NICK_ALREADY ]]; then
    echo "Errore non corrispondente $e" 
    exit 1
fi

# deregistro pippo
./client -l $1 -k pippo -C pippo
if [[ $? != 0 ]]; then
    exit 1
fi
# deregistro pluto
./client -l $1 -k pluto -C pluto
if [[ $? != 0 ]]; then
    exit 1
fi

# registro pippo
./client -l $1 -c pippo
if [[ $? != 0 ]]; then
    exit 1
fi
# registro pluto
./client -l $1 -c pluto
if [[ $? != 0 ]]; then
    exit 1
fi

# pippo e pluto si scambiano files
./client -l $1 -k pippo -S "Ti mando un file":pluto -s ./client:pluto
if [[ $? != 0 ]]; then
    exit 1
fi
./client -l $1 -k pluto -S "Ti mando un file":pippo -s ./chatty:pippo -s ./libchatty.a:pippo
if [[ $? != 0 ]]; then
    exit 1
fi


# invio il segnale per generare le statistiche
kill -USR1 $pid

sleep 1

# termino i client in ascolto
kill -TERM $pid1 $pid2

sleep 1

# invio il segnale per far terminare il server
kill -TERM $pid

sleep 2

r=$(tail -10 ./valgrind_out | grep "ERROR SUMMARY" | cut -d: -f 2 | cut -d" " -f 2)

if [[ $r != 0 ]]; then
    echo "Test FALLITO"
    exit 1
fi

echo "Test OK!"
exit 0


