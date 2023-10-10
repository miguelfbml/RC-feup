










enum Status {RECEIVER = 0, TRANSMITTER = 1 }; 




int llopen(int porta, enum Status status)  {
    switch (status)
    {
    case RECEIVER:
        /* code */









        break;

    case TRANSMITTER:
        /* code */









        break;

    default:
        break;
    }



}



argumentos –porta: COM1, COM2, ...  –flag: TRANSMITTER / RECEIVER 
retorno –identificador da ligação de dados



–valor negativo em caso de erro