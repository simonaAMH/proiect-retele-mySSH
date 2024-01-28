 #include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sqlite3.h>
#include <math.h>
#include <libexplain/execvp.h>

#define PORT 2729
extern int errno;

static int callback(void *data, int argc, char **argv, char **azColName) {
    int *logat = (int *)data;

    if (argc > 0) 
        *logat = 1;
    else 
        *logat = 0;

    return 0;
};

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

void resize_response(char **response, int new_length) 
{
  char *new_response = realloc(*response, (new_length * sizeof(char)));
  if (new_response == NULL) {
    printf("Eroare la realocarea memoriei.\n");
    exit(EXIT_FAILURE);
  }
  *response = new_response;
}

char ** parse_command(char comanda[]) 
{    

    char ** argumente;
    char* temp_comanda = strdup(comanda);
    int n = 0;

    char *token = strtok(comanda, " ");
    while (token != NULL) { //calculam n = nr de argumente pt a sti cata memorie sa alocam
        n++;
        token = strtok(NULL, " ");
    }

    argumente = malloc((n+1) * sizeof(char*)); //alocam memoria
    token = strtok(temp_comanda, " ");

    for (int i = 0; i < n; i++) { //copiem argumentele in vector
        argumente[i] = malloc((strlen(token) + 1) * sizeof(char));
        bzero(argumente[i], (strlen(token) + 1) * sizeof(char));
        strncpy(argumente[i], token, strlen(token));
        token = strtok(NULL, " ");
    }
    
    argumente[n] = NULL;
    return argumente;
}

char ** parse_response(char *raspuns) 
{
    int j = 0;
    int length = strlen(raspuns);
    char **pachete = malloc((floor(length / 500) + 1) * sizeof(char *));

    for (int i = 0; i < length; i += 500) 
    {   
        pachete[j] = malloc(500 * sizeof(char));
        bzero(pachete[j], 500);
        strncpy(pachete[j], raspuns + i, 500);
        j++;
    }

    pachete[j] = NULL;
    free(raspuns);
    return pachete;
}

char ** execute_command(char ** argumente)     
{
    char *raspuns = malloc(500 * sizeof(char));
    bzero(raspuns, (500 * sizeof(char)));
    char buffer[500];
    int pipe_fd[2], status;
    int length = 0;

    if (pipe(pipe_fd) == -1) 
    {
        perror("Error creating pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    switch (pid = fork ())
    {
    case -1:
       perror("Error forking");
       exit(EXIT_FAILURE);
       break;

    case 0:

        for (int i = 0; argumente[i] != NULL; i++) 
        if ((strcmp(argumente[i], ">")) == 0) { //redirect stdout
            close (pipe_fd[0]);
            close (pipe_fd[1]);
            char *output_file = argumente[i+1];
            int output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd == -1) {
                printf("Error opening output file");
                exit(EXIT_FAILURE);
            }
            argumente[i] = NULL;
            dup2(output_fd, 1);
            close(output_fd);
            }

        else if ((strcmp(argumente[i], "<")) == 0) { //redirect stdin
            close (pipe_fd[0]);
            dup2(pipe_fd[1], 1); //outputul il trimite in pipe
            char *input_file = argumente[i+1];
            int input_fd = open(input_file, O_RDONLY);
            if (input_fd == -1) {
                printf("Error opening input file");
                exit(EXIT_FAILURE);
            }
            argumente[i] = NULL;
            dup2(input_fd, 0);
            close(input_fd);
            }

        else if ((strcmp(argumente[i], "2>")) == 0) { //redirect stderr
            close (pipe_fd[0]);
            dup2(pipe_fd[1], 1); //trimiti raspunsul in pipe
            char *error_file = argumente[i+1];
            int error_fd = open(error_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (error_fd == -1) {
                printf("Error opening error file");
                exit(EXIT_FAILURE);
            }
            argumente[i] = NULL;
            dup2(error_fd, 2);
            close(error_fd);
            }

        else if ((strcmp(argumente[i], "&>")) == 0) { // redirect stdout si stderr
            close (pipe_fd[1]);
            close (pipe_fd[0]);
            char *output_error_file = argumente[i + 1];
            int output_error_fd = open(output_error_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_error_fd == -1) {
                printf("Error opening error file");
                exit(EXIT_FAILURE);
            }
            argumente[i] = NULL;
            dup2(output_error_fd, 1);  // redirect stdout to error_fd
            dup2(output_error_fd, 2);  // redirect stderr to error_fd
            close(output_error_fd);
        }

        else
            {
            close (pipe_fd[0]);
            dup2(pipe_fd[1], 1); //stdout si stderr vor fi directionate catre pipe
            dup2(pipe_fd[1], 2);
            }
        
        if (execvp(argumente[0], argumente) < 0) 
        {   
            printf("Comanda nu exista.\n"); //comanda va fi afisata acolo unde a fost redirectionat stdout
            strncat(raspuns, "Comanda nu exista.", strlen ("Comanda nu exista.") + 1);
            char raspuns_temp[500];
            bzero(raspuns_temp, 500);
            sprintf(raspuns_temp, "%s\n", explain_execvp(argumente[0], argumente));
            fprintf(stderr, "%s\n",raspuns_temp);
            exit(EXIT_FAILURE);
        }
        
        break;
    
    default:
    
        int bytes_read;
        close(pipe_fd[1]);
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) //status 0 if exited without a signal
        {
            while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) 
            {
            length = strlen(raspuns) + bytes_read; 
            if(length > strlen(raspuns))
            { 
                resize_response(&raspuns, length);
                fflush(stdout);
            }
            strncat(raspuns, buffer, bytes_read);
            usleep(5);
            fflush(stdout);
            }
           
        } else 
            strncat(raspuns, "Comanda nu a fost executata.", strlen ("Comanda nu a fost executata.") + 1);
        
        
        close(pipe_fd[0]);
        usleep(5);
        return parse_response(raspuns);
        
    }
}

int main ()
{
    struct sockaddr_in server;
    struct sockaddr_in from;
	srand(time(NULL));
    char raspuns[500];
    bzero(raspuns,500);
    char query[200], nume_utilizator[300], parola[200], info_utilizator[500];
	char comanda_decriptata [100];
    int sd, optval = 1, bytes_read = 0, logat = 0, bytes_din_pachet = 0, nr_pachete = 0;
	unsigned int p, q;
    unsigned long long n, private_key, public_key, client_public_key_e, client_public_key_n, comanda_encriptata[100], raspuns_encriptat[500];
    p = generate_prime(100, 500);
    q = generate_prime(100, 500);
    while (p==q)
        p = generate_prime(100, 500);

    n = p * q;
    public_key = public_key_function(p,q);
    private_key = private_key_function (public_key, p, q);
	// printf("\nServer's public_key e: %llu \nServer's public_key_n: %llu \nServer's private_key_d: %llu\n", public_key, n, private_key);
	// fflush(stdout);

	sqlite3 *db;
	char *err_msg = 0;
	int rc = sqlite3_open("users.db", &db);

    if (rc != 0) {
        perror("Eroare la accesarea bazei de date.");
		return errno;
    }

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
    	perror ("[server]Eroare la socket().\n");
    	return errno;
    }
   
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    bzero (&server, sizeof (server));
    bzero (&from, sizeof (from));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl (INADDR_ANY); //host to network, long (pt adresa ip)
    server.sin_port = htons (PORT); //host to network, short (pt port)
    if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
    	perror ("[server]Eroare la bind().\n");
    	return errno;
    }

    if (listen (sd, 1) == -1)
    {
    	perror ("[server]Eroare la listen().\n");
    	return errno;
    }

    while (1)
    {
    	int client;
    	int length = sizeof (from);
    	printf ("[server]Asteptam la portul %d...\n",PORT);
    	fflush (stdout);

    	client = accept (sd, (struct sockaddr *) &from, &length);
		logat = 0;

    	if (client < 0)
    	{
    		perror ("[server]Eroare la accept().\n");
    		continue;
    	}
						//SCHIMB DE CHEI PUBLICE CU CLIENTUL
		write(client, &public_key, sizeof(public_key));
		write(client, &n, sizeof(n));
		read(client, &client_public_key_e, sizeof(client_public_key_e));
		read(client, &client_public_key_n, sizeof(client_public_key_n)); 
		// printf("\nClient's public_key e: %llu \nClient's public_key_n: %llu \n", client_public_key_e, client_public_key_n);
		// fflush(stdout);

    	int pid;
    	if ((pid = fork()) == -1) 
			{
				close(client);
				continue;
			} 
		else if (pid > 0) // parinte
			{
				close(client);
				while(waitpid(-1,NULL,WNOHANG));
				continue;
			} 
		else if (pid == 0) // copil
			{

				close(sd);
				bzero(comanda_decriptata, 100);
				printf ("Asteptam comanda...\n");
				fflush (stdout);
				logat = 0;

			while ((bytes_read = read (client, &comanda_encriptata, sizeof(comanda_encriptata))) > 0) //citim comenzi in mod continuu
                {

				decrypt_text(private_key, n, comanda_encriptata, comanda_decriptata, bytes_read);
				printf("Comanda introdusa de client: %s \n", comanda_decriptata); //afisam comanda
				fflush (stdout);
                char ** pachete;
                nr_pachete = 0;

				if (strcmp(comanda_decriptata, "login") == 0 && logat == 0)  //implementare login
					{
						strncpy(raspuns, "Introduceti numele de utilizator si parola: ", strlen("Introduceti numele de utilizator si parola: "));
						bytes_din_pachet = strlen (raspuns) + 1;
                        nr_pachete = 1;
                        pachete= malloc(1*sizeof(char*));
                        pachete[0] = malloc (bytes_din_pachet * sizeof(char));
                        strncpy(pachete[0], raspuns, (bytes_din_pachet + 1)); //punem raspunsul intr-un pachet
                        write (client, &nr_pachete, sizeof(nr_pachete)); //trimitem nr de pachete
                        write(client, &bytes_din_pachet, sizeof(bytes_din_pachet)); //trimitem nr de bytes de citit
						encrypt_text(client_public_key_e, client_public_key_n, pachete[0], raspuns_encriptat, bytes_din_pachet);
						write (client, &raspuns_encriptat, sizeof(raspuns_encriptat));//trimitem mesaj catre client sa introduca username si parola
						bzero(raspuns, 500);
						bzero(info_utilizator, 500);
						bzero(nume_utilizator, 300);
						bzero(parola, 200);
						
						bytes_read = read(client, &comanda_encriptata, sizeof(comanda_encriptata)); //citim username ul si parola

						decrypt_text(private_key, n, comanda_encriptata, info_utilizator, bytes_read );
						printf("Comanda introdusa  de client: %s\n", info_utilizator);

						sscanf(info_utilizator, "%s %s", nume_utilizator, parola);

						printf("\nUsername introdus de client: %s\nParola introdusa de client: %s\n", nume_utilizator, parola);
						fflush(stdout);

						char *username_to_check = nume_utilizator;
						char *password_to_check = parola;
						bzero(query, 200);
						sprintf(query, "SELECT * FROM users WHERE username='%s' AND password='%s'", username_to_check, password_to_check);

						if (sqlite3_exec(db, query, callback, &logat, &err_msg) == SQLITE_OK) 
							if (logat == 1)
							{	
                                strncpy(raspuns, "V-ati autentificat cu succes.", strlen("V-ati autentificat cu succes.")); 
                            }
							else 
							{
                            	strncpy(raspuns, "Datele introduse sunt incorecte.", strlen("Datele introduse sunt incorecte."));
                            }
					}
				else if (strcmp(comanda_decriptata, "login") == 0 && logat == 1)
                        {
                            strncpy(raspuns, "Sunteti deja autentificat.", strlen("Sunteti deja autentificat."));
                        }
				else if (strcmp(comanda_decriptata, "login") != 0 && logat == 0)
                        {
                            strncpy(raspuns, "Trebuie sa va autentificati pentru a trimite comenzi catre server.", strlen("Trebuie sa va autentificati pentru a trimite comenzi catre server."));
                        }
				else if (strcmp(comanda_decriptata, "logout") == 0 && logat == 1)
					{
                        strncpy(raspuns, "V-ati delogat cu succes.", strlen("V-ati delogat cu succes."));
						logat = 0;
					}
				else if (strcmp(comanda_decriptata, "login") != 0 && logat == 1)
					{
                     if(strstr(comanda_decriptata, "&&") != NULL)
                        {
                            char *cmd1 = strtok(comanda_decriptata, "&&");
                            char *cmd2 = strtok(NULL, "&&");
                            char ** argumente_1 = parse_command(cmd1);
                            char ** argumente_2 = parse_command(cmd2);
                            char ** pachet_cmd1 = execute_command (argumente_1);
                            char ** pachet_cmd2 = execute_command (argumente_2);
                            int i, j, k =0,a=0,b=0;

                            for (i = 0; pachet_cmd1[i] != NULL; i++) 
                                a++;
                            for (i = 0; pachet_cmd2[i] != NULL; i++) 
                                b++;

                            //daca prima comanda nu are output afisam asta si nu trecem la comanda 2
                            if(strncmp(pachet_cmd1[0], "Comanda nu a fost executata.", strlen("Comanda nu a fost executata.")) == 0
                                || strncmp(pachet_cmd1[0], "Comanda nu exista.", strlen("Comanda nu exista.")) == 0 )
                                {
                                    pachete = malloc ((a+1) * sizeof(char*));
                                    for (i = 0; i < a; i++) 
                                    {
                                        pachete[i] = malloc(500 * sizeof(char));
                                        bzero(pachete[i], 500);
                                        strcpy(pachete[i], pachet_cmd1[i]);
                                    }
                                    pachete[a] = NULL;

                                }
                            else //daca prima comanda are un output valid, combinam outputurile celor 2 comenzi
                                {
                                    k = a+b;
                                    pachete = malloc ((k+1) * sizeof(char*)); //un spatiu in plus pt NULL

                                    //imbinam pachete_cmd1 si pachete_cmd2 in pachete[]
                                    for (i = 0; i < a; i++) {
                                        pachete[i] = malloc(500 * sizeof(char));
                                        bzero(pachete[i], 500);
                                        strcpy(pachete[i], pachet_cmd1[i]);
                                    }
                                    i = 0;
                                    for (j = a; j < k; j++) { 
                                        pachete[j] = malloc(500 * sizeof(char));
                                        bzero(pachete[j], 500);
                                        strcpy(pachete[j], pachet_cmd2[i]);
                                        i++;
                                    }

                                    pachete[k] = NULL;
                                }

                            //eliberam memoria pt argumente_1, argumente_2, pachet_cmd1, pachet_cmd2
                            for (int p = 0; p < a; p++)
                                if (pachet_cmd1[p] != NULL) 
                                    free(pachet_cmd1[p]);
                            free(pachet_cmd1);

                            for (int p = 0; p < b; p++)
                                if (pachet_cmd2[p] != NULL) 
                                    free(pachet_cmd2[p]);
                            free(pachet_cmd2);

                            for (int p = 0; argumente_1[p] != NULL; p++)
                                free(argumente_1[p]);
                            free(argumente_1);

                            for (int p = 0; argumente_2[p] != NULL; p++)
                                free(argumente_2[p]);
                            free(argumente_2);
                        }

                        else 
                        if(strstr(comanda_decriptata, "||") != NULL)
                        {
                            char *cmd1 = strtok(comanda_decriptata, "||");
                            char *cmd2 = strtok(NULL, "||");
                            char ** argumente_1 = parse_command(cmd1);
                            char ** argumente_2 = parse_command(cmd2);
                            char ** pachet_cmd1 = execute_command (argumente_1);
                            char ** pachet_cmd2 = execute_command (argumente_2);
                            int i, j, k =0,a=0,b=0;

                            for (i = 0; pachet_cmd1[i] != NULL; i++) 
                                a++;
                            for (i = 0; pachet_cmd2[i] != NULL; i++) 
                                b++;

                            //daca prima comanda nu are un output valid, o afisam pe a doua
                            
                            if(strncmp(pachet_cmd1[0], "Comanda nu a fost executata.", strlen("Comanda nu a fost executata.")) == 0
                                || strncmp(pachet_cmd1[0], "Comanda nu exista.", strlen("Comanda nu exista.")) == 0 )
                                {
                                    pachete = malloc ((b+1) * sizeof(char*));
                                    for (i = 0; i < b; i++) 
                                    {
                                        pachete[i] = malloc(500 * sizeof(char));
                                        bzero(pachete[i], 500);
                                        strcpy(pachete[i], pachet_cmd2[i]);
                                    }
                                    pachete[b] = NULL;

                                }
                            else //o afisam pe prima
                                {
                                    pachete = malloc ((a+1) * sizeof(char*));
                                    for (i = 0; i < a; i++) 
                                    {
                                        pachete[i] = malloc(500 * sizeof(char));
                                        bzero(pachete[i], 500);
                                        strcpy(pachete[i], pachet_cmd1[i]);
                                    }
                                    pachete[a] = NULL;
                                }

                            //eliberam memoria pt argumente_1, argumente_2, pachet_cmd1, pachet_cmd2
                            for (int p = 0; p < a; p++)
                                if (pachet_cmd1[p] != NULL) 
                                    free(pachet_cmd1[p]);
                            free(pachet_cmd1);

                            for (int p = 0; p < b; p++)
                                if (pachet_cmd2[p] != NULL) 
                                    free(pachet_cmd2[p]);
                            free(pachet_cmd2);

                            for (int p = 0; argumente_1[p] != NULL; p++)
                                free(argumente_1[p]);
                            free(argumente_1);

                            for (int p = 0; argumente_2[p] != NULL; p++)
                                free(argumente_2[p]);
                            free(argumente_2);
                        }
                        
                        else 
                        if(strstr(comanda_decriptata, ";") != NULL)
                        {
                            char *cmd1 = strtok(comanda_decriptata, ";");
                            char *cmd2 = strtok(NULL, ";");
                            char ** argumente_1 = parse_command(cmd1);
                            char ** argumente_2 = parse_command(cmd2);
                            char ** pachet_cmd1 = execute_command (argumente_1);
                            char ** pachet_cmd2 = execute_command (argumente_2);
                            int i, j, k =0,a=0,b=0;

                            for (i = 0; pachet_cmd1[i] != NULL; i++) 
                                a++;
                            for (i = 0; pachet_cmd2[i] != NULL; i++) 
                                b++;
                            k = a+b;
                            pachete = malloc ((k+1) * sizeof(char*)); //un spatiu in plus pt NULL

                            //imbinam pachete_cmd1 si pachete_cmd2 in pachete[]
                            for (i = 0; i < a; i++) {
                                pachete[i] = malloc(500 * sizeof(char));
                                bzero(pachete[i], 500);
                                strcpy(pachete[i], pachet_cmd1[i]);
                            }
                            i = 0;
                            for (j = a; j < k; j++) { 
                                pachete[j] = malloc(500 * sizeof(char));
                                bzero(pachete[j], 500);
                                strcpy(pachete[j], pachet_cmd2[i]);
                                i++;
                            }

                            pachete[k] = NULL;

                            //eliberam memoria pt argumente_1, argumente_2, pachet_cmd1, pachet_cmd2
                            for (int p = 0; p < a; p++)
                                if (pachet_cmd1[p] != NULL) 
                                    free(pachet_cmd1[p]);
                            free(pachet_cmd1);

                            for (int p = 0; p < b; p++)
                                if (pachet_cmd2[p] != NULL) 
                                    free(pachet_cmd2[p]);
                            free(pachet_cmd2);

                            for (int p = 0; argumente_1[p] != NULL; p++)
                                free(argumente_1[p]);
                            free(argumente_1);

                            for (int p = 0; argumente_2[p] != NULL; p++)
                                free(argumente_2[p]);
                            free(argumente_2);
                        }

                        else //comanda simpla
                        {
                            char ** argumente = parse_command(comanda_decriptata);
                            pachete = execute_command(argumente);
                            
                            for (int i = 0; argumente[i] != NULL; i++)
                                free(argumente[i]);
                            free(argumente);
                        }
					}

                bytes_din_pachet = strlen (raspuns) + 1; //daca raspunsul nu este outputul unei comenzi
                //ori avem raspunsul copiat in variabila 'raspuns', ori avem un output in char ** pachete[]
                if(bytes_din_pachet != 1) //daca variabila 'raspuns' contine date, le punem intr-un singur pachet
                {
                    nr_pachete = 1;
                    pachete= malloc(1*sizeof(char*));
                    pachete[0] = malloc (bytes_din_pachet * sizeof(char));
                    strncpy(pachete[0], raspuns, bytes_din_pachet);
                }
                else //numaram cate pachete avem de trimis
                {
                    for (int i = 0; pachete[i] != NULL; i++) 
                          nr_pachete++;
                        
                }
            
                write (client, &nr_pachete, sizeof(nr_pachete)); //trimitem nr de pachete
                for (int i = 0; i < nr_pachete; i++) 
                {
                    bytes_din_pachet = strlen(pachete[i]) + 1;
                    write(client, &bytes_din_pachet, sizeof(bytes_din_pachet)); //trimitem nr de bytes din fiecare pachet care vor trebui decriptati
                    encrypt_text(client_public_key_e, client_public_key_n, pachete[i], raspuns_encriptat, bytes_din_pachet);
                    write (client, &raspuns_encriptat, sizeof(raspuns_encriptat));
                }


                for (int i = 0; i < nr_pachete; i++) 
                    free(pachete[i]);
                free(pachete);
				bzero(comanda_decriptata,100);
                bzero(raspuns, 500);
				printf ("\nAsteptam comanda...\n");
				fflush (stdout);

				}//while(read) comenzi

			close (client);
			exit(0);
			}	
    }//while(1)

sqlite3_free(err_msg);
sqlite3_close(db);
return 0;			
}	 