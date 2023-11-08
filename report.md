# Protocolo de ligação de dados
## (1º Trabalho Laboratorial)


#### - Miguel Filipe Brandão Martins Lima up202108659 
#### - Pedro Miguel Martins Romão up202108660         

## Sumário
O objetivo deste projeto é implementar um protocolo de nível de ligação de dados. Para alcançar esse objetivo, será desenvolvida uma aplicação simples de transferência de arquivos que permite acessar arquivos no disco (no lado do transmissor) e armazenar arquivos no disco (no lado do receptor) através da porta série RS-232.

Através deste projeto, conseguimos aplicar o conhecimento adquirido nas aulas teóricas para colocar em prática o protocolo em questão, consolidando conhecimentos como o funcionamento e a eficiência da estratégia Stop-and-Wait.

## 1. Intrudução
O objetivo do trabalho foi desenvolver um protocolo de ligação de dados, de acordo com as especificações do guião, capaz de transferir um ficheiro de um computador para outro através da porta série.
O relatório está divido em secções:
- **Arquitetura** - blocos funcionais e interfaces.
- **Estrutura do código** - principais estruturas de dados, funções e relação com a arquitetura.
- **Casos de uso principais** - lógica do codigo, sequências de chamada de funções.
- **Protocolo de ligação lógica** - Funcionamento da camada de ligação lógica e a sua implementação.
- **Protocolo de aplicação** - Funcionamento da camada de aplicação e a sua implementação.
- **Validação** - Testes efetuados para avaliar a robustez e o correcto funcionamento do protocolo.
- **Conclusões** - síntese da informação apresentada nas secções anteriores e reflexão sobre os objetivos de aprendizagem alcançados.


## 2. Arquitetura

### Blocos funcionais
O projeto foi desenvolvido tendo em conta dois principais blocos lógicos, ```Linklayer``` e a ```Applicationlayer```.


A camada ```LinkLayer``` é responsável pelo funcionamento do protocolo da ligação de dados. É responsavel pelo estabelecimento e término da ligação, pela criação de tramas com informação e o seu envio através da porta série. É também responsável de verificação dos dados enviados e recebidos garantindo que os dados sejam os pretendidos.

A camada ```ApplicationLayer``` utiliza a API da LinkLayer para transferência e receção de pacotes de dados de um ficheiro. É a camada mais proxima do utilizador e nela pode se redifinir o tamanho das tramas de informação, o número de retransmições, e o tempo entre cada retransmição.

### Interface

Para executar o programa é necessário dois terminais, um em cada computador. Um deles corre em modo emissor e o outro em modo recetor.

<'nome do binário'> <'porta'> <'modo'>
- ./program /dev/ttyS10 emissor
- ./program /dev/ttyS11 recetor




## 3. Estrutura do código

As funções e estruturas de dados utilizadas na ```LinkLayer```:
```
// cria ligação com a serial port
int setup(const char *serialPortName);
// estabelece ligação com entre o emissor e recetor
int llopen(const char *porta, enum Status status);
// termina a ligação
int llclose(int fd);
// envia tramas
int llwrite(int fd, unsigned char *buffer, int length);
// lê as tramas
int llread(int fd, unsigned char *buffer);
```

```
//estados das maquinas de estados
enum rec_status
{
    Start,
    flag_rcv,
    a_rcv,
    c_rcv,
    bcc_ok,
    a_tx,
    c_tx,
    STOP,
    read_data,
    found_esc,
    next_esc
};

//modo
enum Status
{
    RECEIVER,
    TRANSMITTER
};
```


As funções utilizadas na ```ApplicationLayer```:
```
//cria um pacote de controlo
unsigned char *MakeCPacket(unsigned char control, unsigned char *filename, long int length, unsigned int *size);
//cria um pacote de dados
unsigned char *MakeDPacket(unsigned char control, unsigned char *data, int length, unsigned int *size);
```

A lógica da AplicationLayer está na função main da application_layer.c pelo que estas funções são apenas auxiliares. O código implementado para o devido funcionamento da application_layer está dentro da função main.


## 4. Casos de uso principais
