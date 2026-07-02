## README.md Compliance Check

Je vais changer l'intraname pour mettre le mien. Je voudrais le rendre un peu plus verbeux pour etre sûre que personne ne nous embête lors de l'evaluation.

## Basic checks 

**Tout est ok.**

• There is a Makefile, the project compiles correctly with the required options, is written in C++, and the
executable is named as expected.

• Ask and check how many poll() (or equivalent) calls are present in the code. There must be only one.

**Yep there is only one.**

• Verify that no call to accept, read/recv, write/send happens before a call to poll() (or equivalent). After these calls, errno should not be used to trigger specific actiona (e.g., like reading again after errno == EAGAIN).

**Yep pas de call to accept, read/recv, write/send avant le poll(). Errno n'est jamais utilisé pour trigger une action.**

• Verify that each call to fcntl() is done as follows: fcntl(fd, F_SETFL, O_NONBLOCK); Any other use of fcntl() is
forbidden

**Correct: fcntl est seulement utilisé pour rendre les sockets non bloquants.**

## Networking

Check the following requirements:
• The server starts and listens on all network interfaces on the port given from the command line.

**Ouais j'ai testé avec localhost et avec l'adresse ip.**

• Using the 'nc' tool, you can connect to the server, send commands, and the server responds.
• Ask the team what is their reference IRC client.
• Using this IRC client, you can connect to the server.
• The server can handle multiple connections simultaneously. The server should not block. It should be able to answer all demands. Conduct some tests with the IRC client and nc simultaneously.
• Join a channel using the appropriate command. Ensure that all messages from one client on that channel are
sent to all other clients who have joined the channel.

**Quand on se connecte avec irssi, il y a des messages un peu bizarres mais jepense que c'est ok ? (comme on n'a pas besoin d'implementer certaines choses comme WHO et WHOIS).**

**Sur irssi on ne voit pas les codes d'erreur envoyés par le serveur mais on peut log ce qu' irssi recoit et prouver que irssi recoit bien le full message et ensuite irssi reformate ce que l'on voit.**

## Networking specials

Network communications can be disturbed by many unusual situations.
• As in the subject, using nc, try to send partial commands. Check that the server responds correctly. With a
partial command sent, ensure that other connections continue to function properly.
• Unexpectedly terminate a client. Then check that the server remains operational for other connections and
any new incoming client.
• Unexpectedly terminate an nc session with just half of a command sent. Check again that the server is not in
an unusual state or blocked.
• Suspend a client (^-Z) connected to a channel. Then flood the channel with another client. The server should
not freeze. When the client is active again, all stored commands should be processed normally. Also, check
for memory leaks during this operation.

**Pour valgrind, je ne sais pas trop comment tester tout ca. Parce que j'arrete le serveur avec ctrl-c et la on voit qu'il n'y a pas de leak mais il y a des trucs dans still reachable. Est ce que je tente de handle le SIGINT pour qu'ensuite on puisse montrer un truc super propre lors de l'eval ?**

**J'ai fais pas mal de bash scripts pour test tout ca, on peut les utilise pendant l'evaluation.**

## Client Commands basic

• With both nc and the reference IRC client, check that you can authenticate, set a nickname, set a username,
and join a channel. This should be fine (you should have already done this earlier).
• Verify that private messages (PRIVMSG) are fully functional with various parameters.

**Toutes les edges cases testées étaient gérées par les serveur.**

**En faisant MSGPRV #test hello banana (donc en oubliant le :), les autres clients recoivent seulement hello et pas hello banana. C'est ce qu'il faut faire non ?**

## Client Commands channel operator

• With both nc and the reference IRC client, check that a regular user does not have the privileges to perform
channel operator actions. Then test with an operator. All channel operation commands should be tested
(deduct one point for each feature that is not working).

**MODE #test +z => On pourrait retourner 472 ERR_UNKNOWNMODE**
**MODE #test +zi => Ca ignore le z and marche pour le i, est ce que on laisse comme ca en mode permissif (je pense c'est ok comme ca) ou on rejette toute la cmd avec 472 en mode strict ?**

**Quand tous les clients quittent un channel, le channel semble continuer à exister. Il faudrait probablement détruire le channel.**

**Tu as reussi a faire marcher bircd ? Je voudrais bien l'utiliser pour comparer mais après moult aventures pour reussir a le décompresser, quand je le lance avec par exemple ./bircd 6667 et que j'essaie de m'authentifier avec un client il ne se passe rien**