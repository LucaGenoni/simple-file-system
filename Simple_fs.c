/*******************************************************************************
  File system.c è il programma di gestione cartelle e file creato per l'esame
  pratico di API

  Richieste:
             Gerarchico con stoccaggio unicamente in memoria principale;
             Struttura ad albero;
             Ogni risorsa univocamente identificata tramite il percorso.

  File       sono foglie dell'albero
             contengono dati, sequenza di byte.

  Cartelle   sono foglie o nodi intermedi;
             hanno dei metadati, tra cui i nomi dei discendenti.
             NB. i nomi li posso memorizzare in vari modi:
                 lista, code, ecc
                 funzione hashing (problematica, come la dovrei fare? 5 classi, per ogni cartella avrei 5 puntatori!)

  Istruzioni ricevute tramite input.
  Risultato mostrato in output.
  Librerie standard.
  
  Alla riga 1357 e 1358 c'è la possibilità di inserire un Test da file

********************************************************************************
********************************************************************************

  Prime osservazioni:
        è più comodo utilizzare il getchar piuttosto che un gets/scanf,
        in questo modo non devo leggere tutta la stringa, ma la leggo a pezzi.
        prima il comando, poi la 1° cartella, 2° e così via

        tutte le istruzioni devono rispettare questa forma:
              exit: exit\n
              find: find parola\n
              write: write /percorso/nome "contenuto"\n
              altri: comando /percorso/nome\n
        se vengono scritti male si scrive "no"
        NB è accettato devono essere accettati gruppi di spazi tra il comando, percorso e contenuto

        è troppo sbatti farne una sola visto che le grammatiche sono diverse!

        ho fatto una struttura controlli per capire meglio i ritorni delle funzioni

        il contenuto è potenzialmente infinito, serve una struttura per gestire
        questo problema, un tempo mi ricordo che se i messaggi avevano più di
        255 caratteri, il cellulare divideva il messaggio in due potrei
        utilizzare un metodo simile per potenzialmente accettare un contenuto
        infinito.
        utilizzo un metodo simile il 256° carattere è sempre \0

        qualsiasi pointer occupa 4 byte, almeno nel mio pc

        creare le cartelle e i file in ordine mi fa risparmiare tempo
        visto che devo controllare che non ci siano doppioni
        vuol dire che devo controllare tutti i nomi dei figli
        ma se io li creo in ordine non ho bisogno di controllare tutti i nomi dei figli
        a meno che non creo una funzione hashing o la risorsa è maggiore di tutte le altre
        in teoria visto che i figli sono massimo 1024
        controllare 1024 nomi è una complessità temporale costante



        utilizzando una lista normale(e non uan tabella hash o alberi binari particolari)
        è molto più facile eseguire la find perchè appena trovo una corrispondenza posso scriverla (i test sono spesso anche in ordine alfabetico)
        e risparmio spazio in memoria
********************************************************************************
********************************************************************************

*******************************************************************************/
/*******************************************************************************
                              Librerie standard
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
                   definizione costanti massime di default
*******************************************************************************/

#define MAXLUNG 255 // partendo da 1 il 256° è \0, partendo da 0 255° è \0
#define MAXALT 255 // lo 0 è dedicato alla cartella radice
#define MAXFIGLI 1024 //prima avevo impostato <= nella creazione delle cartelle e file ma ne crava una in più
#define BLOCCO 255 //grandezza massima di ogni pezzo di contenuto convenienza con unsigned char


/*******************************************************************************
                           definizione di strutture
*******************************************************************************/

typedef enum {FALSE,TRUE,CONTINUA,SPAZIO,VIRGOLETTE,SIMBOLI,VUOTO,VUOTOTRATTO,VUOTOFINALE} controlli;

typedef struct n_figli{
        unsigned  c:11;				//occupa 11 per sapere se ci sono cartelle figlie
        unsigned  f:11;				//occupa 11 per sapere se ci sono file figlie
        unsigned non_usato:10;		//10 bit non usati ma occupati per riempire la PAROLA del processore
        }t_figli;					//in teoria occupa 3 byte, ma in pratica ne occupa 4 :/ il calcolatore funziona così, a parole (potenze di 2)
        
typedef struct _blocco_c{                    //un blocco di contenuto, da 255 caratteri
        char *con;                           //*nom=pointer a nome ln=lungezza nome *c= ponter a contenuto                                    //nc=lunghezza contenuto
        struct _blocco_c *next;              //*f=ponter a file fratelli
        }t_blocco;
        
typedef struct _percorso{
        char *p;
        unsigned int lp;
        struct _percorso *next;
        }t_percorso;

typedef struct _cartella{
        char *nom;                          //*nom= pointer a nome ln= lunghezza nome pointer
        unsigned char ln;                   // ln= numero caratteri, non è compreso \0 ogni volta bisogna aggiungerlo
        t_figli nf;                          //nf=struttura per numero figli         2byte
        struct _cartella *f,*fc;          //p=padre f=fratelli fc=figli cartelle  3xponiter a lista cartelle
        struct _file *ff;                    //ff=figli file                         ponter a lista file
        } t_cartella ;

typedef struct _file{
        char *nom;                       //*nom=pointer a nome ln=lungezza nome *c= ponter a contenuto
        unsigned char ln;                // ln= numero caratteri, non è compreso \0 ogni volta bisogna aggiungerlo
        long unsigned int nc;                              //nc=lunghezza contenuto
        struct _file *f;                     //*f=ponter a file fratelli
        struct _blocco_c *c;
        }t_file;

/*******************************************************************************
                           Prototipi di funzioni
*******************************************************************************/

t_cartella* trova_fratello_c(t_cartella*, char*);
t_file* trova_fratello_f(t_file*, char*);

/* 
	trim from standardin
	char readed		return
	'/'				CONTINUA
    '"'				VIRGOLETTE
    '\n'			TRUE
    ALTRO			SIMBOLI       	
*/
controlli free_spazi();

/* INPUT_COMANDO
        n=0
        ciclo fino a raggiungere 15 sono stato già buono visto che il comando più lungo è create_dir con 10 caratteri +1 per la conclusione quindi 11
              prendi il carattere se è
              \n
                     l'input è vuoto         VUOTOFINALE
                     l'input è pieno         TRUE
              ' '
                     l'input è vuoto         VUOTO
                     l'input è pieno         SPAZIO
              LETTERE MINUSCOLE + _
                     inseriscile nella stinga alla posizione n
                     incrementa n
              ALTRI CARATTERI
                                             SIMBOLI
        FINE CICLO
        troppo lungo per essere un comando
                                             FALSE
*/
controlli input_comando(char *);

/* lettura percorso
        n=0
        ciclo fino <= MAXLUNG quindi arriva a 255 se inserisce un carattere finale ok, se no esce dal giro e restituisce false.
                              si poteva fare anche unsigned char, ma dopo dovevo controllare il carattere conclusivo fuori dal ciclo, sbatti
              prendi il carattere se è
              \n
                     l'input è vuoto         VUOTOFINALE
                     l'input è pieno         TRUE
              ' '
                     l'input è vuoto         VUOTO
                     l'input è pieno         SPAZIO
              '/'
                     l'input è vuoto         VUOTOTRATTO
                     l'input è pieno         CONTINUA
              LETTERE MINUSCOLE e MAIUSCOLE e minuscole
                     inseriscile nella stinga alla posizione n
                     incrementa n
              ALTRI CARATTERI
                                             SIMBOLI
        FINE CICLO
        troppo lungo per essere un nome di file o cartella
                                             FALSE
*/
controlli input_percorso(char *, unsigned int *);

//se trova una corrispondenza restituisce il pointer alla cartella/file

t_cartella* trova_cartella(t_cartella *, char *);
t_file* trova_file(t_file *, char *);

/*serve una variate della funzione trova fratello, restituisce il fratello che
punta alla risorsa, non la risorsa stessa.*/
//Il controllo sul primo figlio avviene al di fuori della funzione
t_cartella* trova_fratello_c(t_cartella *, char *);
t_file* trova_fratello_f(t_file *, char *);

/*cancella contenuto di un file
           procedimento: ricevi il blocco
                         libera i caratteri del blocco
                         segna il ponter al 2°blocco
                         libera il 1°blocco
                         se il 2°blocco non è NULL richiama la funzione*/
void cancella_contenuto(t_blocco *); 


/* segui il percorso
         CICLO(fino a quando altezza<255) cerca il padre e restituisce il nome del figlio,
                                          quindi va bene l'altezza massima della cartella padre di 254
                                          il figlio avrà altezza 255
         c_nome=lettura percorso
               CONTINUA
                       procedere con il percorso
                       se non esiste il figlio con quel nome
                              libera l'input
                              return NULL                                        NULL
               SPAZIO
                       L'input è finito
                       controlli di freespazio
                                 TRUE
                                        SE c_write
                                           FALSE
                                                non essendo una write l'input è ok
                                                return PADRE                     PADRE
                                           TRUE
                                                manca il contenuto
                                                return NULL                      NULL
                                 VIRGOLETTE
                                        SE c_write
                                           FALSE
                                                non può esserci una virgoletta in una funzione normale
                                                pulisci l'input
                                                return PADRE                     PADRE
                                           TRUE
                                                la sintassi è corretta
                                                return PADRE                      PADRE
                                 CONTINUA, SIMBOLI
                                         non può esserci uno spazio,simboli dentro un percorso
                                         pulisci l'input
                                         return NULL                               NULL

               TRUE
                    SE c_write
                       FALSE
                            non essendo una write l'input è ok
                            return PADRE                                           PADRE
                       TRUE
                           manca il contenuto
                           return NULL                                             NULL
               VUOTOFINALE
                       errore non è accettata la stringa vuota
               VUOTO, VUOTOTRATTO, SIMBOLI, FALSE
                       errore stringa vuota, input scorretti, troppo lungo
                       serve pulire l'input
      FINE CICLO
      è stata superata l'altezza
      libera spazi
      return NULL
*/    
t_cartella * segui_p(t_cartella *,unsigned char *,char *,unsigned int *, controlli *);
  

/*
create
      descrizione: crea il file nel percorso inserito in input devo controllare i nomi!
                   se ci sono problemi stampa "no\n"
      INIZIO
      *altezza=0;
      c_wrtie=FALSE;
      segui il percorso(*nome,*altezza,*c_write)
      se restituisce NULL ci sono stati problemi scrivi "no"
         se no vuol dire che ho la cartella padre
               se ci sono risorse con lo stesso nome ERRORE!(prima controlla file, poi cartelle)
                     Se no crea il file e scrivi "OK"*/    
void create(t_cartella *padre);               
controlli crea_file_1(t_cartella *,char* , unsigned int *);
controlli crea_file_n(t_file *,char* , unsigned int *); 

/*
create_dir
      descrizione: crea il file nel percorso inserito in input devo controllare i nomi!
                   se ci sono problemi stampa "no\n"
      INIZIO
      *altezza=0;
      c_wrtie=FALSE;
      segui il percorso(*nome,*altezza,*c_write)
      se restituisce NULL ci sono stati problemi scrivi "no"
         se no vuol dire che ho la cartella padre
               se ci sono file con lo stesso nome ERRORE!(prima controlla file, poi cartelle)
                     Se ci sono cartelle con lo stesso nome ERRORE!
                        se no voglio la posizione della
*/   
void create_dir (t_cartella *);
controlli crea_cartella_1(t_cartella *,char* , unsigned int *);    
controlli crea_cartella_n(t_cartella *,char* , unsigned int *); 

/* INPUT_BLOCCO
        n=0
        ciclo fino a raggiungere i BLOCCO caratteri
              LETTERE+SPAZIO
                            li inserisco nella stinga, alla posizione n
                            incremento n
              "
                    posizione della stinga metto \0
                    ritorno VIRGOLETTE
              \n
                    ritorno FALSE
              ALTRO
                    ritotno SIMBOLI
        FINE CICLO
        posizione della stinga metto \0
        ritorno CONTINUA
*/
controlli input_blocco(char *, unsigned char *);

/* INPUT_CONTENUTO
        ciclo infinito, per un contenuto infinito ;D
              crea la stringa (Lunghezza della stringa +1 per il \0)
                   se c'è un errore scrivi no e ritorna FALSE
              c_cont=scrivi il blocco, tendo conto dei caratteri scritti
              c_cont può essere
                     CONTINUA
                             incremento il contatore dei caratteri totali sommando i BLOCCO caratteri
                             creo un nuovo blocco
                             cambio il blocco corrente e lo inizzializzo tutto con NULL
                     VIRGOLETTE
                               bisogna controllare che dopo le " ci sia solo l'a capo
                               c_cont=free
                                     TRUE
                                         finisce l'input del contenuto in modo corretto
                                         aumento il contatore di quanti caratteri ho letto
                                         realloco la memoria della stringa a c_car_blocco+1 sempre per il \0
                                     CONTINUA, SIMBOLI, VIRGOLETTE
                                         non ho un a capo dopo le " quindi l'input è errato
                                         faccio la free fino all'a capo
                                         ritonro FALSE
                     SIMBOLI
                            c'è un carattere non valido
                            libero l'input fino all'a capo
                            ritorno FALSE
                     FALSE
                          ho già letto a capo
                          ritorno FALSE
        se finisco il ciclo è un errore stranissimo
        ritorno FALSE
*/
controlli input_contenuto(t_blocco *, long unsigned int* ,controlli *);

/*
write
     descrizione: segue il percorso ed entra nel file fa un check di spazi
      INIZIO
      *altezza=0;
      c_wrtie=FALSE;
      num_caratteri=0;
      segui il percorso(*nome,*altezza,*c_write)
      se restituisce NULL ci sono stati problemi scrivi no
         se no vuol dire che ho la cartella padre
               se ci sono file con lo stesso nome TROVATO il FILE!
                  a questo punto l'input scrive all'interno del contenuto
                  se c_write==TRUE
                           scrittura nel blocco!!!!!funzione
                           è finito l'input
                           somma al contatore il contatore interno
                           se il contatore è 0 FALSE
                              se no
                     FALSE errore input!
 */
void write_f (t_cartella *);

/*
read
      descrizione: raggiunge il file indicato e scrive il contenuto
      INIZIO
      c_write=0;
      segui il percorso(*nome,*altezza,*c_write)
      se restituisce NULL ci sono stati problemi scrivi "no"
         se no vuol dire che ho la cartella padre
               se ci sono file con lo stesso nome scrivi il contenuto
               se no scrivi "no"
 */
void read_f(t_cartella *);

/*
delete
      descrizione: raggiunge il padre
                   se è un file restituisci il pointercancella il contenuto
                                         poi il file
                   se no se è una cartella controlla se non ha figli cancella
                                                     se ha figli ERROR
                         se no ERROR
 */
/*CANCELLA FILE
           procedimento: dammi il padre e il nome e io ti cancello il file

*/
controlli elimina_file(t_cartella *, char *);

/*CANCELLA CARTELLA
           procedimento: dammi il padre e il nome e io ti cancello la cartella

*/
controlli elimina_cartella(t_cartella *, char *);

/*
elimina_tutto
             descrizione :riceve la cartella ed elimina tutti i figli
             procedimento:
                          riceve la cartella
                          elimina tutti i file:
                                               elimina sempre il primo figlio, quello puntato da padre->ff
                          fino a quando il numero dei file e/o il ponter di ff è NULL
                          elimina tutte le cartelle una per una
                                  quando trova una cartella con figli richiama questa funzione e applicala a questa cartella con figli
                          fino a quando il numero delle cartelle e/o il pointer di fc è NULL

*/
void elimina_figli(t_cartella *);
void deletes(t_cartella *);

/*
delete_r
         descrizione: cancella la risorsa e tutte le risorse figlie

         procedimento:
                      segui il percorso
                      se è un file eliminalo easy
                      se è una cartella, se non ha figli elimina easy
                                         se ha figli file eliminali tutti, con funzione ricorsiva elimindando sempre il 1°
                                         se ha figli cartelle ricorsività, sarebbe utile una funzione che si basa ricorsivamente solo sul 1° figlio

        */

void delete_r(t_cartella *);


/* ricerca_figli


   parametri: padre è la cartella dove cercare
              nome e lung sono quello che devo cercare
              precedente tine memoria del percorso fino a quel punto

 descrizione:
             ciclo per i file
             se vede che il file combacia segna nel controllo SPAZIO
             fine ciclo file

             controlla le cartelle
                       se il controllo è SPAZIO paragona il file con la cartella
                                                se il file è z lo stampa
                                                controllo TRUE
                                         FALSE controllo il nome della cartella
                                                se la cartella è quella giusta
                                                   stampa cartella
                                                   controllo TRUE
                       se la cartella ha figli controllali richiamando la funzione ricerca
                       se il ritorno è TRUE segna  in figli TRUE
             fine ciclo cartella
             se controllo è SPAZIO stampa il file
             se figli è TRUE  controllo è TRUE
             ritorna controllo



*/
controlli ricerca_figli(t_cartella *, char* , unsigned int *, t_percorso* );

/* FIND
   l'idea originale era memorizzare in una lista tutti i percorsi trovati, quindi mi servivano 2 liste
   una che tenesse in memoria il percorso attuale e l'altra che memorizzasse il percorso di quelle trovate
   a questo punto riordinavo la lista delle stringhe trovate
   e poi le stampavo.

   eliminando la creazione "casuale" e introducendo l'ordine nella creazione ho potuto risparmiare tempo e spazio

   creo la stinga per il nome
   ricevo l'input con la funzione percorso
          questi if non sono annidati.
          VUOTO
               oh spazio, rileggemi che magari dopo la parolina magica

          SPAZIO
                oh spazio, dinuovo
                libera dagli spazi
                       TRUE
                           c'erano solo infiniti spazi...
                       CONTINUA, VIRGOLETTE, SIMBOLI
                                 frocio chi legge questo input
                                 scrivi no
                                 ritorna
          VUOTOFINALE
                     errore stampa no
                     figa e mettimela la parola, la stinga vuota non l'accetto
                     ritorna
          TRUE
              è stato inserito il nome e poi basta
              CERCA
              RIORDINA
              STAMPA


          VUOTOTRATTO,  CONTINUA,  SIMBOLI, FALSE
               input lungo, scorreto mai e poi mai
               free spazi fino a capo
               stampa no
               ritorna
*/
void find(t_cartella *);

 /*******************************************************************************
                     Dichiarazione variabili globali
*******************************************************************************/
FILE *standardin,*standardout;

/*******************************************************************************
                                   Main
*******************************************************************************/
/* MAIN
        crea la stringa per il comando, vari controlli, la cartella radice e la inizzializza
        sistema gli standard, per fare le prove in locale
        inizia il CICLO INFINITO
               ricevi l'input,
               VUOTO
                        continua a richiedere l'input
               SPAZIO
                        se è find entra nella find
                        se no richiedi una pulizia degli spazi
                              CONTINUA
                                      c'è uno / quindi è un percorso
                                      compara le scritte con i comandi
                                              se non trova riscontri scrivi no
                              TRUE
                                      se è exit allora exit
                                      se no scrivi no
                              VIRGOLETTE, SIMBOLI
               TRUE
                        se è exit allora exit
                        se no scrivi no
               VUOTOFINALE
                        semplicemente l'input è sbagliato
                        scrive no
               SIMBOLI, FALSE
                        è richiesta una pulizia dell'input
                        scrive no
        FINE CICLO

*/
int main(){
    t_cartella radice;//cartella di partenza
    char c, comando[11];
    controlli c_com;
    int i=0,*p;    
    t_figli provadim;
    
    //crazione cartella di base;
    radice.nom=NULL;
    radice.nf.c=0;
    radice.nf.f=0;
    radice.ln=2;
    radice.nom=(char*) malloc (radice.ln*sizeof(char));
    strcpy(radice.nom,"/");
    radice.f=NULL;
    radice.fc=NULL;
    radice.ff=NULL;
		
    standardin=stdin;
    standardout=stdout;
//	standardin=fopen("TEST/vines.in","r"); //per testare in locale, si possono utilizzare alnche altri tipi di test inventati
//	standardout=fopen("output.out","w");   //per vedere l'output e paragonarlo con i file .out
//	if(!standardin || !standardout) return -1;

    while(1){
       c_com=VUOTO;
       while(c_com==VUOTO) c_com=input_comando(comando); //accetta gli spazi iniziali
      //  fprintf(stdout,"n %d tipo: %s controllo %d\n",i++, comando, c_com);
        if (c_com==SPAZIO){
             if (strcmp(comando,"find")==0){// non posso fare dei controlli, se no rischio ti togliere la prima lettera
                 find(&radice);
             }else{
                 c_com=free_spazi(); //se ci sono spazi e non è find leggi fino a che non finiscono gli spazi
                 if (c_com==CONTINUA){
                     if (strcmp(comando,"create")==0) create(&radice);
                     else if (strcmp(comando,"create_dir")==0) create_dir(&radice);
                     else if (strcmp(comando,"read")==0) read_f(&radice);
                     else if (strcmp(comando,"write")==0) write_f(&radice);
                     else if (strcmp(comando,"delete")==0) deletes(&radice);
                     else if (strcmp(comando,"delete_r")==0) delete_r(&radice);
                     else {// non combacia con le funzioni esistenti
                          while (c_com!=TRUE) c_com=free_spazi();
                          fprintf(standardout,"no\n");
                     };
                 }else if (c_com==TRUE){//C_com==SIMBOLI o FALSE
                       if (strcmp(comando,"exit")==0) exit(0);
                       else fprintf(standardout,"no\n");
                 }else {
                       while (c_com!=TRUE) c_com=free_spazi();
                       fprintf(standardout,"no\n");
                 };
             };
        }else if (c_com==TRUE){
              if (strcmp(comando,"exit")==0) exit(0);
              else fprintf(standardout,"no\n");

        }else if (c_com==VUOTOFINALE){
              fprintf(standardout,"no\n");

        }else {//sono simboli o l'input è più lungo di 11 caratteri
              while (free_spazi()!=TRUE);//resetta fino all'a capo
              fprintf(standardout,"no\n");
        };
    };//FINE CICLO INFINITO
}
/*******************************************************************************
                     Dichiarazione funzioni, istruzioni
*******************************************************************************/

controlli free_spazi(){
          char c;
          while(1){
              c=getc(standardin);
              if (c!=' '){
                 if(c=='/') return (CONTINUA);
                 else if (c=='"') return (VIRGOLETTE);
                      else if(c=='\n') return (TRUE);
                           else return (SIMBOLI);
              };
          };
}

controlli input_comando(char *comando){
          char c;
          unsigned int n=0;
          while(n<11){
              c=getc(standardin);
              if(c=='\n') {
                  if(n==0) return (VUOTOFINALE);
                  else {
                      comando[n]='\0';
                      return (TRUE);
                  };
              }else if (c==' ') {
                       if(n==0) return (VUOTO);
                       else {
                          comando[n]='\0';
                          return (SPAZIO);
                       };
              }else if ( c=='_' ||(c>=97 && c<=122)){
                              comando[n]=c;
                              n++;
              }else return (SIMBOLI);
        };
        return (FALSE);
}

controlli input_percorso(char *nome, unsigned int *n){
          char c;
          (*n)=0;
          while((*n)<=MAXLUNG){
              c=getc(standardin);
              if (c=='\n'){
                 if((*n)==0) return (VUOTOFINALE);
                 else {
                      nome[(*n)]='\0';
                      (*n)++;
                      return (TRUE);
                 };
              }else if (c==' '){
                    if((*n)==0) return (VUOTO);//cartella, file senza nome finisce il percorso
                    else {
                         nome[(*n)]='\0';
                         (*n)++;
                         return (SPAZIO);
                    };
              }else if (c=='/'){
                    if((*n)==0) return (VUOTOTRATTO);//cartella, file senza nome
                    else {
                         nome[(*n)]='\0';
                         (*n)++;
                         return (CONTINUA);
                    };
              }else if ((c>=48 && c<=57) || (c>=65 && c<=90) ||(c>=97 && c<=122)){
                    nome[(*n)]=c;
                    (*n)++;
              } else return(SIMBOLI);
          };
          return (FALSE);// superato il limite di 255
}

t_cartella* trova_cartella(t_cartella *c_corrente, char *nome){
        while (c_corrente!=NULL){        
            if( strcmp(c_corrente->nom,nome)==0) return (c_corrente);
            else c_corrente=c_corrente->f;
        };
        return(c_corrente);
}

t_file* trova_file(t_file *c_corrente, char *nome){
    while (c_corrente!=NULL){          
        if( strcmp(c_corrente->nom,nome)==0)return (c_corrente);
        else c_corrente=c_corrente->f;
    };
    return(c_corrente);
}

t_cartella* trova_fratello_c(t_cartella *c_corrente, char *nome){
        while (c_corrente->f!=NULL){
              if( strcmp(c_corrente->f->nom,nome)==0) return (c_corrente);
                  else c_corrente=c_corrente->f;
        };
        return(c_corrente->f);
}

t_file* trova_fratello_f(t_file *c_corrente, char *nome){
        while (c_corrente->f!=NULL){
              if( strcmp(c_corrente->f->nom,nome)==0) return (c_corrente);
                  else c_corrente=c_corrente->f;
        };
        return(c_corrente->f);
}

void cancella_contenuto(t_blocco *blocco){
          t_blocco *next;
          while(blocco!=NULL){
              next=blocco->next;
              free(blocco->con);
              free(blocco);
              blocco=next;
          };
}

t_cartella * segui_p(t_cartella *padre,unsigned char *altezza,char *nome,unsigned int *lung, controlli *c_write){
            controlli c_nome=CONTINUA;
            while ((*altezza)<MAXALT){
          //      fprintf(standardout,"stampa segui percorso***********************\nnome: %s\nnumero figli: %d (%d+%d)\npointer fratello: %x\npointer figli cartelle: %x file: %x\n\n",padre->nom,padre->nf.c+padre->nf.f,padre->nf.c,padre->nf.f,padre->f,padre->fc,padre->ff);
                c_nome=input_percorso(nome,lung);
          //      fprintf(standardout,"input stringa: %s\nlunghezza: %u\ncontrollo %d\n",nome,*lung,c_nome);
                if(c_nome==CONTINUA){
                    (*altezza)++;
                    padre=trova_cartella(padre->fc, nome);
                    if( padre==NULL) {
                        while(free_spazi()!=TRUE);
                        return (NULL);
                    };
                }else if (c_nome==SPAZIO){
                      c_nome=free_spazi();
                      if (c_nome==TRUE){
                          if(*c_write==FALSE) return (padre);
                          else return (NULL);

                      }else if(c_nome==VIRGOLETTE){
                            if(*c_write==TRUE) return (padre);
                            else {
                                 while(free_spazi()!=TRUE);
                                 return (NULL);
                            };
                      }else {
                            while(free_spazi()!=TRUE);
                            return (NULL);
                      };
                }else if(c_nome==TRUE){
                      if(*c_write==FALSE) return (padre);
                      else return (NULL);

                }else if(c_nome==VUOTOFINALE) return (NULL);
                else{
                     while(free_spazi()!=TRUE);
                     return (NULL);
                };
            };
            while(free_spazi()!=TRUE);
            return (NULL);
}

controlli crea_file_1(t_cartella *padre,char* nome, unsigned int *lung){
     t_file *file=NULL;
        if (padre->nf.f!=0) file=padre->ff;

        padre->ff= (t_file *) malloc(sizeof(t_file));
        if( padre->ff==NULL) return(FALSE);
        padre->ff->ln=*lung;

        padre->ff->nom=(char *) malloc (((unsigned int)(padre->ff->ln+1))*sizeof(char));
        if( padre->ff->nom==NULL) return(FALSE);
        strcpy(padre->ff->nom,nome);

        padre->ff->c=NULL;
        padre->ff->nc=0;
        padre->ff->f=file;
        return(TRUE);
}

controlli crea_file_n(t_file *padre,char* nome, unsigned int *lung){//padre in realtà è il fratello precedente
        t_file *file;
        char paragone;
        while (padre!=NULL){
              //la nuova cartella è l'ultima, la creo
           if (padre->f==NULL){

                 padre->f= (t_file *) malloc(sizeof(t_file));
                 if( padre->f==NULL) return (FALSE);
                 padre->f->ln=*lung;

                 padre->f->nom=(char *) malloc (((unsigned int)(padre->f->ln+1))*sizeof(char));
                 if( padre->f->nom==NULL) return (FALSE);
                 strcpy(padre->f->nom,nome);

                 padre->f->nc=0;
                 padre->f->c=NULL;
                 padre->f->f=NULL;
            //   fprintf(standardout,"stampa cartella appena creata\nnome: %s\nlunghezza nome: %u\nnumero figli: %d (%d+%d)\npointer fratello: %x\npointer figli cartelle: %x file: %x\n\n",padre->fc->nom,padre->fc->ln,padre->fc->nf.c+padre->fc->nf.f,padre->fc->nf.c,padre->fc->nf.f,padre->fc->f,padre->fc->fc,padre->fc->ff);
                 return (TRUE);
           }else if((paragone=strcmp(nome,padre->f->nom))<0){
                 file=padre->f;

                 padre->f= (t_file *) malloc(sizeof(t_file));
                 if( padre->f==NULL) return (FALSE);
                 padre->f->ln=*lung;

                 padre->f->nom=(char *) malloc (((unsigned int)(padre->f->ln+1))*sizeof(char));
                 if( padre->f->nom==NULL) return (FALSE);
                 strcpy(padre->f->nom,nome);

                 padre->f->nc=0;
                 padre->f->c=NULL;
                 padre->f->f=file;
              // fprintf(standardout,"stampa cartella appena creata\nnome: %s\nlunghezza nome: %u\nnumero figli: %d (%d+%d)\npointer fratello: %x\npointer figli cartelle: %x file: %x\n\n",padre->fc->nom,padre->fc->ln,padre->fc->nf.c+padre->fc->nf.f,padre->fc->nf.c,padre->fc->nf.f,padre->fc->f,padre->fc->fc,padre->fc->ff);
                 return (TRUE);
           }else if (paragone==0){
                 return (SIMBOLI);//doppione
           };
           padre=padre->f; //scala
     };
     return(VUOTO);//RAGGIUNTO IL LIMITE
}

void create(t_cartella *padre){
         unsigned char altezza=0;
         unsigned int lung;
         controlli c_write=FALSE;
         char nome[MAXLUNG+1], paragone;//serve per il \0
         t_cartella *cartella;
         c_write=FALSE;
         padre=segui_p(padre,&altezza,nome,&lung,&c_write);
         if (padre==NULL){
            c_write=FALSE;
         }else{
           if((padre->nf.c+padre->nf.f)<MAXFIGLI){
             cartella=trova_cartella(padre->fc,nome);
             if (cartella==NULL){
                 if (padre->nf.f==0){// non ci sono cartelle
                    c_write=crea_file_1(padre,nome,&lung);
                 }else if ((paragone=strcmp(nome,padre->ff->nom))<0){//la prima cartella è maggiore rispetto all'ordine lessicografico rispetto alla nuova
                    c_write=crea_file_1(padre,nome,&lung);
                 }else if (paragone>0){//cerco dove inserire la cartella
                    c_write=crea_file_n(padre->ff,nome,&lung);
                 }else c_write=FALSE;//il file esiste già
             }else c_write=FALSE;//il file ha lo stesso nome di una cartella
           }else c_write=FALSE;//superato il limite di figli
         };
      //   fprintf(standardout,"controllo: %d  ",c_write);
         if (c_write==TRUE){
              padre->nf.f++;
              fprintf(standardout,"ok\n");
         }else fprintf(standardout,"no\n");
         return;
}

controlli crea_cartella_1(t_cartella *padre,char* nome, unsigned int *lung){
     t_cartella *cartella=NULL;
        if (padre->nf.c!=0) cartella=padre->fc;

        padre->fc= (t_cartella *) malloc(sizeof(t_cartella));
        if( padre->fc==NULL) return(FALSE);
        padre->fc->ln=*lung;

        padre->fc->nom=(char *) malloc (((unsigned int)(padre->fc->ln+1))*sizeof(char));
        if( padre->fc->nom==NULL)return(FALSE);
        strcpy(padre->fc->nom,nome);

        padre->fc->nf.c=0;
        padre->fc->nf.f=0;
        padre->fc->fc=NULL;
        padre->fc->ff=NULL;
        padre->fc->f=cartella;
     // fprintf(standardout,"stampa cartella appena creata\nnome: %s\nlunghezza nome: %u\nnumero figli: %d (%d+%d)\npointer fratello: %x\npointer figli cartelle: %x file: %x\n\n",padre->fc->nom,padre->fc->ln,padre->fc->nf.c+padre->fc->nf.f,padre->fc->nf.c,padre->fc->nf.f,padre->fc->f,padre->fc->fc,padre->fc->ff);
        return(TRUE);
}

controlli crea_cartella_n(t_cartella *padre,char* nome, unsigned int *lung){//padre in realtà è il fratello precedente
        t_cartella *cartella;
        int paragone;
        while (padre!=NULL){
              //la nuova cartella è l'ultima, la creo
           if (padre->f==NULL){

                 padre->f= (t_cartella *) malloc(sizeof(t_cartella));
                 if( padre->f==NULL) return (FALSE);
                 padre->f->ln=*lung;

                 padre->f->nom=(char *) malloc (((unsigned int)(padre->f->ln+1))*sizeof(char));
                 if( padre->f->nom==NULL) return (FALSE);
                 strcpy(padre->f->nom,nome);

                 padre->f->nf.c=0;
                 padre->f->nf.f=0;
                 padre->f->fc=NULL;
                 padre->f->ff=NULL;
                 padre->f->f=NULL;
            //   fprintf(standardout,"stampa cartella appena creata\nnome: %s\nlunghezza nome: %u\nnumero figli: %d (%d+%d)\npointer fratello: %x\npointer figli cartelle: %x file: %x\n\n",padre->fc->nom,padre->fc->ln,padre->fc->nf.c+padre->fc->nf.f,padre->fc->nf.c,padre->fc->nf.f,padre->fc->f,padre->fc->fc,padre->fc->ff);
                 return (TRUE);
           }else if((paragone=strcmp(nome,padre->f->nom))<0){
                 cartella=padre->f;

                 padre->f= (t_cartella *) malloc(sizeof(t_cartella));
                 if( padre->f==NULL)return (FALSE);
                 padre->f->ln=*lung;

                 padre->f->nom=(char *) malloc (((unsigned int)(padre->f->ln+1))*sizeof(char));
                 if( padre->f->nom==NULL) return (FALSE);
                 strcpy(padre->f->nom,nome);

                 padre->f->nf.c=0;
                 padre->f->nf.f=0;
                 padre->f->fc=NULL;
                 padre->f->ff=NULL;
                 padre->f->f=cartella;
              // fprintf(standardout,"stampa cartella appena creata\nnome: %s\nlunghezza nome: %u\nnumero figli: %d (%d+%d)\npointer fratello: %x\npointer figli cartelle: %x file: %x\n\n",padre->fc->nom,padre->fc->ln,padre->fc->nf.c+padre->fc->nf.f,padre->fc->nf.c,padre->fc->nf.f,padre->fc->f,padre->fc->fc,padre->fc->ff);
                 return (TRUE);
           }else if (paragone==0){
                 return (SIMBOLI);//doppione
           };
           padre=padre->f; //scala
     };
     return(VUOTO);
}

void create_dir (t_cartella *padre){
         unsigned char altezza=0;
         unsigned int lung;
         controlli c_write=FALSE;//non è la write
         char nome[MAXLUNG+1],paragone;//il +1 serve per il \0
         t_file *file;
     //    fprintf(standardout,"_______________________________________________________________________________\n");
         padre=segui_p(padre,&altezza,nome,&lung,&c_write);

         if (padre==NULL){
            c_write=FALSE;//non esiste il percorso
         }else{
           if((padre->nf.c+padre->nf.f)<MAXFIGLI){

            // cartella=trova_cartella(padre->fc,nome); le cartelle devono essere ordinate! non va bene questa ricerca
             file=trova_file(padre->ff,nome);
             if (file==NULL){//se non ci sono doppioni tra i file posso controllare le cartelle
                 if (padre->nf.c==0){// non ci sono cartelle

                    c_write=crea_cartella_1(padre,nome,&lung);

                 }else if ((paragone=strcmp(nome,padre->fc->nom))<0){//la prima cartella è maggiore rispetto all'ordine lessicografico rispetto alla nuova
                    c_write=crea_cartella_1(padre,nome,&lung);
                 }else if (paragone>0){
                    c_write=crea_cartella_n(padre->fc,nome,&lung);
                 }else c_write=FALSE;//la cartella esiste già
             }else c_write=FALSE;//esiste un file con quel nome
           }else c_write=FALSE;//troppi figli
         };
   //      fprintf(standardout,"controllo: %d  ",c_write);
         if (c_write==TRUE){
              padre->nf.c++;
              fprintf(standardout,"ok\n");
         }else fprintf(standardout,"no\n");
         return;
}

controlli input_blocco(char *contenuto, unsigned char *n){
          char c;
          (*n)=0;
          while((*n)<BLOCCO){
              c=getc(standardin);
            //  fprintf(standardout,"input : %c\n",c);
              if(c=='"') {
                    contenuto[(*n)]='\0';
                    return(VIRGOLETTE);//bisogno di free
              }else if ((c>=48 && c<=57) || (c>=65 && c<=90) ||(c>=97 && c<=122) || c==' '){
                    contenuto[(*n)]=c;
                    (*n)++;
              }else if(c=='\n') return(FALSE);//input errato
              else return (SIMBOLI);// input errato, con bisogno di free
          };
          contenuto[(*n)]='\0';// al posto 256° mette il segno di fine stringa
          return (CONTINUA);// superato il limite di 255
}

controlli input_contenuto(t_blocco *blocco, long unsigned int* n_caratteri,controlli *c_write){
          unsigned char n_car_blocco=0;
          controlli c_cont;
          while(1){
              n_car_blocco=0;
              blocco->con=(char*) malloc ((BLOCCO+1)*sizeof(char));
              if(blocco->con==NULL){
                  fprintf(standardout,"no\n");
                  return(FALSE);
              };
              c_cont=input_blocco(blocco->con,&n_car_blocco);
          //    fprintf(stdout,"Parte blocco: %s\nNumero caratteri: %d\nControllo: %d",blocco->con,n_car_blocco,c_cont);
              if (c_cont==CONTINUA){//non ho raggiunto le " ma la fine del blocco
                 (*n_caratteri)= (*n_caratteri)+BLOCCO;
                 blocco->next=(t_blocco*) malloc(sizeof(t_blocco));
                 if( blocco->next==NULL){
                     fprintf(standardout,"no\n");
                     return(FALSE);
                 };
                 blocco=blocco->next;
                 blocco->con=NULL;
                 blocco->next=NULL;
              }else if(c_cont==VIRGOLETTE){
                    c_cont=free_spazi();
                    if (c_cont==TRUE){//dopo le virgolette c'è un a capo
                        (*n_caratteri)= (*n_caratteri)+n_car_blocco;
                        blocco->con=(char*) realloc (blocco->con,(n_car_blocco+1)*sizeof(char));
                        return (TRUE);
                    }else{//ci sono altri simboli, non va bene
                        while(free_spazi()!=TRUE);
                        return (FALSE);
                    };
              }else if(c_cont==SIMBOLI){//ho bisogno di una pulizia
                             while(free_spazi()!=TRUE);
                             return (FALSE);
              }else return(FALSE);//l'utlimo caso è obbligato ad essere \n che quindi è un errore prima delle "
          };
          fprintf(standardout,"no\n");  //in teoria non raggiunge mai questa stringa
          return(FALSE);
}

void write_f (t_cartella *padre){
         //variabili per seguire il percorso
         unsigned char altezza=0;
         unsigned int lung;
         controlli c_write=TRUE;
         char nome[MAXLUNG+1];
         //variabili per leggere il contenuto
         long unsigned int n_caratteri=0;
         t_file *file;
         t_blocco *new_blocco;
         padre=segui_p(padre,&altezza,nome,&lung,&c_write);
         if (padre==NULL){
            fprintf(standardout,"no\n");
            return;
         }else{
             file=trova_file(padre->ff,nome);
             if (file==NULL){
                fprintf(standardout,"no\n");
                c_write=FALSE; //non è stato trovato il file quindi devo liberare l'input
                while(c_write!=TRUE) c_write=free_spazi();
                return;
             }else {
                 new_blocco=(t_blocco *)malloc(sizeof(t_blocco));
                 new_blocco->con=NULL;
                 new_blocco->next=NULL;
                 c_write=input_contenuto(new_blocco,&n_caratteri,&c_write);//c_write assume un altro significato
                 if(c_write==TRUE){
                     cancella_contenuto(file->c);
                     file->c=new_blocco;
                     fprintf(standardout,"ok %lu\n",n_caratteri);
                     return;
                 }else if(c_write==FALSE){
                         fprintf(standardout,"no\n");
                         cancella_contenuto(new_blocco);
                         return;
                 };

              };
         };
}

void read_f(t_cartella *padre){
     //variabili per seguire il percorso
     unsigned char altezza=0;
         unsigned int lung;
     controlli c_write=FALSE;
     char nome[MAXLUNG+1];
     //variabili per leggere il contenuto
     t_file *file;
     t_blocco *blocco;
     padre=segui_p(padre,&altezza,nome,&lung,&c_write);
     if (padre==NULL){
         fprintf(standardout,"no\n");
         return;
     }else{
          file=trova_file(padre->ff,nome);
          if (file==NULL){
             fprintf(standardout,"no\n");
             return;
          }else {
                fprintf(standardout,"contenuto ");
                blocco=file->c;
                while(blocco!=NULL){
                    fprintf(standardout,"%s",blocco->con);
                    blocco=blocco->next;
                }
                fprintf(standardout,"\n");
          };
     };
}

controlli elimina_file(t_cartella *padre, char *nome){
          t_file *file,*fratello;
          if(strcmp(padre->ff->nom,nome)==0){ // il file da cancellare è il primo figlio!
               padre->nf.f--;
               file=padre->ff;  //file è il file da eliminare, memorizzo il suo poniter
               padre->ff=file->f;// assegno al padre il pointer al fratello di file
               //il file non ha più "legami" compromettenti
               cancella_contenuto(file->c);//libera la memoria del contenuto del file
               //libera la memoria di tutto il file
               //printf("file eliminato: %s\n",file->nom);
               free(file->nom);
               free(file);
               return (TRUE);
             }else{
                 fratello=trova_fratello_f(padre->ff,nome);
                 if (fratello!=NULL){//il file da cancellare è nell'elenco dei fratelli
                       padre->nf.f--;
                       file=fratello->f;  //file è il file da eliminare, memorizzo il suo poniter
                       fratello->f=file->f;// assegno al padre il pointer al fratello di file
                       //il file non ha più "legami" compromettenti
                       cancella_contenuto(file->c);//libera la memoria del contenuto del file
                       //libera la memoria di tutto il file
                       //printf("file eliminato: %s\n",file->nom);
                       free(file->nom);
                       free(file);
                       return (TRUE);
                 }else {
                       return (FALSE);
                 };
             };
}

controlli elimina_cartella(t_cartella *padre, char *nome){
          t_cartella *cartella;
          if (strcmp(padre->fc->nom,nome)==0){// la cartella da cancellare è il primo figlio!
                   if ((padre->fc->nf.c+padre->fc->nf.f)==0){
                     padre->nf.c--;
                     cartella=padre->fc;  //cartella è la cartella da eliminare, memorizzo il suo poniter
                     padre->fc=cartella->f;// assegno al padre il pointer al fratello di cartella
                     //la cartella non ha più "legami" compromettenti
                     //libera la memoria di tutto il file
                     //printf("cartella eliminata: %s\n",cartella->nom);
                     free(cartella->nom);
                     free(cartella);

                     return (TRUE);
                   }else{// la cartella non può essere eliminata, perchè ha dei figli
                     return (FALSE);
                   };
                }else{
                    cartella=trova_fratello_c(padre->fc,nome);
                    if( cartella!=NULL){//il file da cancellare è nell'elenco dei fratelli
                        if ((cartella->f->nf.c+cartella->f->nf.f)==0){
                           padre->nf.c--;//il pointer padre non mi serve più, cambio il suo scopo
                           padre=cartella->f;  //padre è la cartella da eliminare, memorizzo il suo poniter
                           cartella->f=padre->f;// assegno al fratello di cartella il pointer del fratello di padre
                           //la cartella, padre, non ha più "legami" compromettenti
                           //libera la memoria di padre
                           //printf("cartella eliminata: %s\n",cartella->nom);
                           free(padre->nom);
                           free(padre);
                           return (TRUE);
                        }else {
                              return (FALSE);
                        };
                    }else return (FALSE);
                };
}

void elimina_figli(t_cartella *padre){
          t_file *file;
          t_cartella *cartella;

          while (padre->ff!=NULL){ //ci sono file, bisogna eliminarli
               padre->nf.f--;
               file=padre->ff;  //file è il file da eliminare, memorizzo il suo poniter
               padre->ff=file->f;// assegno al padre il pointer al fratello di file
              // fprintf(standardout,"file eliminato: %s\n",file->nom);

               //il file non ha più "legami" compromettenti
               cancella_contenuto(file->c);//libera la memoria del contenuto del file
               //printf("file eliminato: %s\n",file->nom);
               //libera la memoria di tutto il file
               free(file->nom);
               free(file);
          };
          while (padre->fc!=NULL){
                //prima elimino i figli della cartella
                if((padre->fc->nf.c+padre->fc->nf.f)!=0) elimina_figli(padre->fc);
                //figli eliminati
                padre->nf.c--;
                cartella=padre->fc;  //cartella è la cartella da eliminare, memorizzo il suo poniter
                padre->fc=cartella->f;// assegno al padre il pointer al fratello di cartella
                //la cartella non ha più "legami" compromettenti
              //  fprintf(standardout,"cartella eliminata: %s\n",cartella->nom);
                //libera la memoria di tutta la cartella
                free(cartella->nom);
                free(cartella);
          };
          //printf("eliminati tutti i figli\n");
          return;
}

void deletes(t_cartella *padre){
     //variabili per seguire il percorso
     unsigned char altezza=0;
     unsigned int lung;
     controlli c_write=FALSE;
     char nome[MAXLUNG+1];
     //variabili per leggere il contenuto

     padre=segui_p(padre,&altezza,nome,&lung,&c_write);
     if (padre==NULL){
         fprintf(standardout,"no\n");
         return;
     }else{
         if(padre->ff!=NULL){// ci sono dei file controlliamoli
             c_write=elimina_file(padre,nome);
             if (c_write==TRUE){
                fprintf(standardout,"ok\n");
                return;
             };
         };//potrebbe essere tra le cartelle
         if (padre->fc!=NULL){// ci sono delle cartelle, controlliamole
             c_write=elimina_cartella(padre,nome);
             if (c_write==TRUE){
                fprintf(standardout,"ok\n");
                return;
             };
         }; //non ci sono neanche delle cartelle
         fprintf(standardout,"no\n");
         return;
     };
}

void delete_r(t_cartella *padre){
     //variabili per seguire il percorso
     unsigned char altezza=0;
     unsigned int lung;
     controlli c_write=FALSE;
     char nome[MAXLUNG+1];
     //variabili per leggere il contenuto
     t_cartella *cartella;
     padre=segui_p(padre,&altezza,nome,&lung,&c_write);
     //printf("nome padre: %s\n",padre->nom);
     if (padre==NULL){
         fprintf(standardout,"no\n");
         return;
     }else{
         if(padre->ff!=NULL){// la risorsa potrebbe essere un file quindi
             c_write=elimina_file(padre,nome);
             if (c_write==TRUE){
                fprintf(standardout,"ok\n");
                return;
             };
         };
         //la risorsa potrebbe essere una cartella
         if (padre->fc!=NULL){// ci sono delle cartelle, controlliamole
            //printf("inizia ricerca\n");
            if (strcmp(padre->fc->nom,nome)==0){
               //printf("eliminazione del prima cartella figlio\n");
               if((padre->fc->nf.c+padre->fc->nf.f)!=0) {
                   elimina_figli(padre->fc);
               };
               //printf("figli della cartella %s sono stati eliminati\n",padre->fc->nom);
               padre->nf.c--;
               cartella=padre->fc;  //cartella è la cartella da eliminare, memorizzo il suo poniter
               padre->fc=cartella->f;// assegno al padre il pointer al fratello di cartella
               //la cartella non ha più "legami" compromettenti
               //lbera la memoria di tutta la cartella
               free(cartella->nom);
               free(cartella);
               fprintf(standardout,"ok\n");
               return;
            }else{
                cartella=trova_fratello_c(padre->fc,nome);//
                if ( cartella==NULL){
                  fprintf(standardout,"no\n");
                   return;
                }else{
                    //printf("entra nell'eliminazione\n");
                    if((cartella->f->nf.c+cartella->f->nf.f)!=0) {
                        elimina_figli(cartella->f);
                    };
                    //printf("figli della cartella %s sono stati eliminati\n",cartella->f->nom);
                    padre->nf.c--; //padre non mi serve più
                    padre=cartella;
                    cartella=cartella->f;  //cartella è la cartella da eliminare, memorizzo il suo poniter
                    padre->f=cartella->f;// assegno al padre il pointer al fratello di cartella
                    //la cartella non ha più "legami" compromettenti
                    //libera la memoria di tutta la cartella
                    free(cartella->nom);
                    free(cartella);
                    fprintf(standardout,"ok\n");
                    return;
                };
            };
         }; //non ci sono neanche delle cartelle
         fprintf(standardout,"no\n");
         return;
     };
}

controlli ricerca_figli(t_cartella *padre, char* nome, unsigned int *lung, t_percorso* precedente){
          t_file *file;
          t_cartella *cartella;

          controlli beccato=FALSE, figli=FALSE;

          file=padre->ff;
          cartella=padre->fc;

          precedente->next=(t_percorso *) malloc(sizeof(t_percorso));
          precedente->next->p=(char *) malloc ((precedente->lp+(unsigned int)MAXLUNG) * sizeof(char));  //lo creo grande abbastanza da contenere qualsiasi nome
          strcpy(precedente->next->p,precedente->p);
       //   fprintf(standardout,"\n\ncerca in cartella %s\nil percorso ricevuto e %s\nQuello attuale %s\n",padre->nom,precedente->p,attuale->p);

          while (beccato==FALSE && file!=NULL){ //se non è stato trovato un file ed esiste controlla il nome
                if(strcmp(file->nom,nome)==0) beccato=SPAZIO; //trovato il file con il nome uguale
                else file=file->f; //scalamento file
          };

          while (cartella!=NULL){//se ha figli cerco all'interno della cartella
                if (beccato==SPAZIO){// se non c'erano file che combaciavano
                    //Se il nome del file è più piccolo del nome della cartella
                    if(strcmp(nome,cartella->nom)<0){
                          fprintf(standardout,"ok %s%s\n",precedente->p,nome);
                          beccato=TRUE;
                    };
                }else if(beccato==FALSE){
                      if(strcmp(nome,cartella->nom)==0){
                          fprintf(standardout,"ok %s%s\n",precedente->p,cartella->nom);
                          beccato=TRUE;
                      };
                };
                if((cartella->nf.c+cartella->nf.f)!=0) {
                    //cambio solo la parte di stringa della cartella
                    strcpy((precedente->next->p+precedente->lp-1),cartella->nom); //scrivo dopo tutto il percorso precedente subito dopo / così sovrascrivo su \0
                    strcat(precedente->next->p,"/");// avendo sovrascritto su \0 mi manca un carattere che lo occupo con lo /
                    precedente->next->lp=precedente->lp+cartella->ln;
                    if ((ricerca_figli(cartella,nome,lung,precedente->next))==TRUE) figli=TRUE;
                    //faccio la ricerca nei figli e se ritorna vero all'interno della cartella ho trovato dei risultati, quindi lo segno
                };

                cartella=cartella->f;//scalamento al fratello
          };
          if(beccato==SPAZIO){// se ho fatto il controllo con tutte le cartelle e non mi ha ancora scritto il file, scrivilo adesso
              fprintf(standardout,"ok %s%s\n",precedente->p,nome);
              beccato=TRUE;
          };
          if (figli==TRUE) beccato=figli;// se il controllore figli è vero, anche beccato è vero
          //precedente->next non mi serve più quindi devo liberare lo spazio
          free(precedente->next->p);
          free(precedente->next);
          //libero precedente->next che non mi serve più, tengo pulita la memoria
          return(beccato);
}

void find(t_cartella *padre){
     char *nome;
     unsigned int lung=0;
     controlli c_nome=VUOTO;
     t_percorso path;


     //allocazione in memoria
     nome=(char*) malloc((MAXLUNG+1)*sizeof(char));
     if(nome==NULL){
        fprintf(standardout,"no\n");
        return;
     };
     //input togliendo gli spazi iniziali e finali
     while (c_nome==VUOTO) c_nome=input_percorso(nome,&lung);
     if (c_nome==SPAZIO) c_nome=free_spazi();

     //riallocazione della memoria per il nome, in lung c'è già lo spazio per il \0
     nome=(char*) realloc(nome,(lung)*sizeof(char));//sistemo la lunghezza del nome da cercare

   //   fprintf(standardout,"nome: %s\nlunghezza %d\ncontrollo %d\n",nome,lung,c_nome);
     if (c_nome==TRUE){
        //istruzione corretta quindi posso cercare
        //creazione del percorso

        path.lp=padre->ln;//uno per \0
        path.p=(char *) malloc ((path.lp)*sizeof(char));
        path.next=NULL;
        strcpy(path.p,padre->nom);

        //RICERCA TRA TUTTE LE CARTELLE
        c_nome=ricerca_figli(padre,nome,&lung,&path); //non mi interessa memorizzare l'ultimo pacchetto, forse per il riordino è carino come idea
        if (c_nome==FALSE) fprintf(standardout,"no\n");
     }else if (c_nome==VUOTOFINALE){
           fprintf(standardout,"no\n");
     }else {
           while (free_spazi()!=TRUE);
           fprintf(standardout,"no\n");
     };
     //libero il contenuto di path e il nome
     free(path.p);
     free(nome);

     return; //return finale per tutto ;D
}
