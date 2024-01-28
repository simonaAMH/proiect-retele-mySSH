// struct sockaddr_in {
//     short            sin_family;   // e.g. AF_INET
//     unsigned short   sin_port;     // e.g. htons(3490)
//     struct in_addr   sin_addr;     // see struct in_addr, below
//     char             sin_zero[8];  // zero this if you want to
// };

// struct in_addr {
//     unsigned long s_addr;  // load with inet_aton()
// };

//conv adresa IP intr un sir de caractere la care adaugam portul
//inet_ntoa ->Network address to ascii
//inet_aton ->ascii to Network address

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>

extern int errno;
int port;

bool is_prime(int n) {
    if (n < 1) return false;
    if (n == 2) return true;
    for (int i =3; i<= (n/2);i++)
        if(n%i==0)
            return false;

    return true;
}

int generate_prime(int min, int max) {
    int r = rand(); 
    int result = min + (r % (max - min + 1));
    while (is_prime(result) != 1)
    {
        r = rand(); 
        result = min + (r % (max - min + 1));
    }
    return result;
}

unsigned long long gcd(unsigned long long a, unsigned long long b) {
   if (b == 0) 
       return a;
   
   return gcd(b, a % b);
}

unsigned long long modular_inverse( unsigned long long e, unsigned long long phi)
{
    for (int d = 3; d < phi; d++)
        if ((d*e) % phi == 1)
            return d;
       
perror ("Eroare la calcularea lui d");
return -1;
}

unsigned long long modpow(unsigned long long a, unsigned long long b, unsigned long long m) {
   unsigned long long result = 1;
   while (b > 0) {
       if (b % 2 == 1 ) {
           result = (result * a) % m;
       }
       a = (a * a) % m;
       b /= 2;
   }
   return result;
}

unsigned long long public_key_function (unsigned int p, unsigned int q)
{
    unsigned long long phi_n, e, temp;
        phi_n = (p-1)*(q-1); //functia lui euler pt n (cate nr prime cu n sunt)
        
        temp = rand(); 
        e = 3 + (temp % ((phi_n - 1) - 3 + 1)); //generam un nr random e intre 1 si phi_n
        while ((gcd(e, phi_n) != 1) || (e >= phi_n)) //daca nu e co-prim cu phi_n sau e mai mare se genereaza altul
        {
            temp = rand(); 
            e = 3 + (temp % (phi_n - 3 + 1));
        }
    
    return e;
}

unsigned long long private_key_function (unsigned long long publicKey, unsigned int p, unsigned int q)
{
 unsigned long long phi_n, d;
 phi_n = (p-1) * (q-1);
 d = modular_inverse (publicKey, phi_n);

 return d;
}

void encrypt_text (unsigned long long public_key_e, unsigned long long public_key_n, char text_to_encrypt[], unsigned long long encrypted_text[], int length)
{
    unsigned long long ascii_encoded[length];
    for ( int i = 0; i < length; i++) 
    {
        ascii_encoded[i] = (int)text_to_encrypt[i];
        encrypted_text[i] = modpow(ascii_encoded[i], public_key_e, public_key_n);
    }
}

void decrypt_text (unsigned long long private_key_d, unsigned long long private_key_n, unsigned long long text_to_decrypt[], char decrypted_text[], int length)
{   
    unsigned long long ascii_coded[length];
    for ( int i = 0; i < length; i++) 
    {
        ascii_coded[i] = modpow(text_to_decrypt[i], private_key_d, private_key_n);
        decrypted_text[i] = (char) ascii_coded[i];
    }
}


int main (int argc, char *argv[])
{
  int sd, bytes_read = 0;
  int nr_pachete = 0, bytes_din_pachet = 0;
  srand(time(NULL));
  struct sockaddr_in server;
  char comanda[100];
  char **pachete;

  unsigned int p, q;
  unsigned long long n, private_key, public_key, server_public_key_e, server_public_key_n, raspuns_encriptat[500], comanda_encriptata[100];
  p = generate_prime(100, 500);
  q = generate_prime(100, 500);
  while (p==q)
      p = generate_prime(100, 500);

  n = p * q;
  public_key = public_key_function(p,q);
  private_key = private_key_function (public_key, p, q);
  // printf("\nClient's public_key e: %llu \nClient's public_key_n: %llu \nClient's private_key_d: %llu\n", public_key, n, private_key);
  // fflush(stdout);

  if (argc != 3)
    {
      printf ("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }
  port = atoi (argv[2]);

  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Eroare la socket().\n");
      return errno;
    }
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]); //inet_addr -> conv din string in struct in_addr.s_addr
  server.sin_port = htons (port); //host to network, short
  
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("Eroare la connect.\n");
      return errno;
    }

  read(sd, &server_public_key_e, sizeof(server_public_key_e)); //schimb de chei publice
  read(sd, &server_public_key_n, sizeof(server_public_key_n));
  write(sd, &public_key, sizeof(public_key));
  write(sd, &n, sizeof(n));
  // printf("\nServer's public_key e: %llu \nServer's public_key_n: %llu \n", server_public_key_e, server_public_key_n);
  // fflush(stdout);


//trimitem o prima comanda pt care sa putem primi un prim raspuns si apoi sa functioneze in bucla while(1)
  printf ("Introduceti o comanda: "); 
  fflush (stdout);
  bzero (comanda, 100);
  bytes_read = read(0, comanda, sizeof(comanda));
  if (bytes_read  <= 0)  //citim de la tastatura comanda
    {
        perror("Eroare la read de la client.\n");
        return errno;
    }
  comanda[strcspn(comanda, "\n")] = '\0';
  if (strncmp(comanda, "quit\n", strlen("quit")) == 0) 
      {
          printf("Serverul se inchide.\n");
          exit(0);
      }
  encrypt_text(server_public_key_e, server_public_key_n, comanda, comanda_encriptata, bytes_read); 
  if (write (sd, &comanda_encriptata, sizeof(comanda_encriptata)) <= 0) //trimitem comanda
    {
      perror ("Eroare la write spre server.\n");
      return errno;
    }

  bzero (comanda, 100);


   while(1)
    {
    nr_pachete = -1;
    bytes_read = read(sd, &nr_pachete, sizeof(nr_pachete)); //citim nr pachete primite ca raspuns
    if (bytes_read < 0)
    {
      perror ("Eroare la citirea nr de pachete de la server.\n");
      return errno;
    }

    printf("nr pachete de citit de la server: %d\n", nr_pachete);
    fflush(stdout);
    bzero(raspuns_encriptat, 500);
    pachete = malloc(nr_pachete * sizeof(char *)); //alocam spatiu pt pachete

    for (int i = 0; i < nr_pachete; i++) 
    {
      bytes_read = read(sd, &bytes_din_pachet, sizeof(bytes_din_pachet)); //citim nr de bytes de decriptat
      if (bytes_read < 0)
      {
        perror ("Eroare la citirea nr de bytes din pachet.\n");
        return errno;
      }
      pachete[i] = malloc(bytes_din_pachet * sizeof(char));
      bzero(pachete[i], bytes_din_pachet);
      bytes_read = read(sd, &raspuns_encriptat, sizeof(raspuns_encriptat)); //citim fiecare pachet
      if (bytes_read < 0)
      {
        perror ("Eroare la citirea unui rasp encriptat de la server.\n");
        return errno;
      }
      decrypt_text(private_key, n, raspuns_encriptat, pachete[i], bytes_din_pachet);
      printf("%s", pachete[i]);
      fflush(stdout);
      bzero(raspuns_encriptat, 500);
    }

   
    if (strncmp(pachete[0], "Introduceti numele de utilizator si parola: ", strlen("Introduceti numele de utilizator si parola: ")) != 0 )
    {  
        printf ("\nIntroduceti o comanda: "); //afisam mesajul doar daca nu trb sa introducem datele de login
        fflush (stdout);
    }
    

    bytes_read = read(0, comanda, sizeof(comanda)); //citim urmatoarea comanda
    if (bytes_read  <= 0)  
      {
          perror("Eroare la read de la client.\n");
          return errno;
      }

    comanda[strcspn(comanda, "\n")] = '\0';
    if (strncmp(comanda, "quit\n", strlen("quit")) == 0) 
        {
            printf("Serverul se inchide.\n");
            break;
        }
    encrypt_text(server_public_key_e, server_public_key_n, comanda, comanda_encriptata, bytes_read); //encriptam comanda
    if (write (sd, &comanda_encriptata, sizeof(comanda_encriptata)) <= 0) //trimitem comanda
      {
        perror ("Eroare la write spre server.\n");
        return errno;
      }

    // for( int i=0;i< bytes_read;i++)
    // printf("Comanda_encriptata[%d] introdusa: %lld \n", i, comanda_encriptata[i]);
    // printf("Comanda decriptata introdusa: %s\n", comanda); 
    // fflush(stdout);

    for (int i = 0; i < nr_pachete; i++)  //eliberam memoria
        free(pachete[i]);

    free(pachete);
    bzero (comanda, 100); 
  }

  close (sd);
}