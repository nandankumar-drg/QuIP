
extern void creat_pipe(QSP_ARG_DECL  const char *name, const char* command, const char* rw);

extern void close_pipe(QSP_ARG_DECL  Pipe *pp);

extern void sendto_pipe(QSP_ARG_DECL  Pipe *pp,const char* text);

extern void readfr_pipe(QSP_ARG_DECL  Pipe *pp,const char* varname);

