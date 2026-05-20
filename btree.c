#include <stdio.h>
#include <stdlib.h>

typedef int TipoChave;
typedef int bool;
#define true 1
#define false 0
#define T 3

typedef struct {
  TipoChave chave;
} Registro;

// NOVA ESTRUTURA DO NÓ DA ÁRVORE B+
typedef struct auxNo {
  int numChaves;
  bool folha;
 
  // Chaves:
  // Nos internos: servem apenas como índices de roteamento.
  // Nas folhas: são as chaves dos registros reais.
  TipoChave chaves[2*T-1];

  union {
      // Campos EXCLUSIVOS de Nós Internos
      struct {
          struct auxNo* filhos[2*T];
      } interno;
      // Campos EXCLUSIVOS de Nós Folha
      struct {
          Registro regs[2*T-1]; // Os dados satélites/reais
          struct auxNo* prox;   // Ponteiro para a lista encadeada
      } folha;
  } info;
 
} No;

typedef struct{
  No* raiz;
} ArvB;

/* Função de inicialização adaptada */
void inicializa(ArvB* a){
  No* novo = (No*) malloc(sizeof(No));
  novo->numChaves = 0;
  novo->folha = true;
  novo->info.folha.prox = NULL; // Importante: inicializa o fim da lista
  a->raiz = novo;
}


typedef struct auxNo{
  int numChaves;
  bool folha;
  Registro regs[2*T-1];
  struct auxNo* filhos[2*T];  // poderiamos alocar dinamicamente sob demanda
} No;

typedef struct{
  No* raiz;
} ArvB;



void exibeNo(No* atual){
  int x;
  printf("Chaves no No atual [%p]: %i\n", atual, atual->numChaves);
  for(x=0;x<atual->numChaves;x++){
    printf("%i ", atual->regs[x].chave);
  }
  printf("\n");
}

int altura(No* atual){
  if (atual->folha) return 0;
  int x, temp, max = 0;  
  for(x=0;x<=atual->numChaves;x++){
    temp = altura(atual->filhos[x]);
    if (temp>max) max = temp;
  }
  return max+1;
}


void exibeEnderecos(No* atual){
  int x;
  printf("Links no No atual [%p]: %i\n", atual, atual->numChaves+1);
  for(x=0;x<atual->numChaves+1;x++){
    printf("%p ", atual->filhos[x]);
  }
  printf("\n");
}

void exibeArvore(No* atual, int nivel){
  printf("Nivel: %i - ", nivel);
  exibeNo(atual);
  if (atual->folha) return;
  int i;
  for (i=0;i<=atual->numChaves;i++) exibeArvore(atual->filhos[i], nivel+1);
}


void salvarNoDisco(No* no){
  printf("O no com endereco %p seria salvo no disco.\n", no);
}

void lerDoDisco(No* no){
  printf("O no com endereco %p seria lido do disco.\n", no);
}


bool buscaRegistroAux(No* atual, TipoChave ch, Registro* reg){
  int i = 0;
  while (i < atual->numChaves && ch > atual->regs[i].chave) i++;
  if (i < atual->numChaves && ch == atual->regs[i].chave){
    *reg = atual->regs[i];
    return true;
  }else{
    if(!atual->folha){
      return buscaRegistroAux(atual->filhos[i], ch, reg);
    }
  }
  return false;
}

bool buscaRegistro(ArvB* a, TipoChave ch, Registro* reg){
  return buscaRegistroAux(a->raiz, ch, reg);
}



void divideNoFilho(No* x, int i, No* y){
  No* novo = (No*) malloc(sizeof(No));
  novo->folha = y->folha;
  novo->numChaves = T - 1;
  int j;
  for (j=0; j<T-1; j++) novo->regs[j] = y->regs[j+T];
  if (!y->folha)
    for (j=0; j<T; j++) novo->filhos[j] = y->filhos[j+T];
  y->numChaves = T - 1;
  for (j=x->numChaves; j>i; j--) x->filhos[j+1] = x->filhos[j];
  x->filhos[i+1] = novo;
  for (j=x->numChaves-1; j>=i; j--) x->regs[j+1] = x->regs[j];
  x->regs[i] = y->regs[T-1];
  x->numChaves++;
  salvarNoDisco(y);
  salvarNoDisco(novo);
  salvarNoDisco(x);
}

void insereNoNaoCheio(No* atual, Registro* reg){
  int i = atual->numChaves-1;
  if (atual->folha){
    while (i>=0 && reg->chave < atual->regs[i].chave){
      atual->regs[i+1] = atual->regs[i];
      i--;
    }
    atual->regs[i+1] = *reg;
    atual->numChaves++;
    salvarNoDisco(atual);
  }else{
    while (i>=0 && reg->chave < atual->regs[i].chave) i--;
    i++;
    lerDoDisco(atual->filhos[i]);
    if(atual->filhos[i]->numChaves==2*T-1){
      divideNoFilho(atual, i, atual->filhos[i]);
      if (reg->chave > atual->regs[i].chave) i++;
    }
    insereNoNaoCheio(atual->filhos[i], reg);
  }
}


void insere(ArvB* a, Registro* reg){
  printf("##### Inserindo: %i\n", reg->chave);
  if (a->raiz->numChaves < 2*T-1) insereNoNaoCheio(a->raiz, reg);
  else{
    No* novo = (No*) malloc(sizeof(No));
    No* r = a->raiz;
    a->raiz = novo;
    novo->numChaves = 0;
    novo->folha = false;
    novo->filhos[0] = r;
    divideNoFilho(novo, 0, r);
    insereNoNaoCheio(novo, reg);
  }
}


int main(){

  ArvB a1;
  inicializa(&a1);
  exibeNo(a1.raiz);

  Registro r1;
  r1.chave = 11;
  insere(&a1, &r1);
  exibeNo(a1.raiz);

  r1.chave = 5;
  insere(&a1, &r1);
  r1.chave = 22;
  insere(&a1, &r1);
  exibeNo(a1.raiz);

  r1.chave = 2;
  insere(&a1, &r1);
  r1.chave = 12;
  insere(&a1, &r1);
  exibeNo(a1.raiz);

  r1.chave = 15;
  insere(&a1, &r1);
  exibeNo(a1.raiz);
  exibeEnderecos(a1.raiz);

  exibeArvore(a1.raiz,1);


  r1.chave = 21;
  insere(&a1, &r1);

  exibeArvore(a1.raiz,1);

  int v;
  for (v=10;v<220; v+=10){
    r1.chave = v;
    insere(&a1, &r1);
    exibeArvore(a1.raiz,1);
  }

  printf("Altura da arvore: %i\n", altura(a1.raiz));

  ArvB a2;
  inicializa(&a2);
  for (v=100;v>0; v--){
    r1.chave = v;
    insere(&a2, &r1);
  }
  for (v=100;v<=200; v++){
    r1.chave = v;
    insere(&a2, &r1);
  }
  exibeArvore(a2.raiz,1);
  printf("Altura da arvore: %i\n", altura(a2.raiz));

  for (v=201;v>=0; v--){
    if (buscaRegistro(&a2, v, &r1)) printf("Encontrei: %i\n", v);
    else printf("Nao encontrei: %i\n", v);
  }



  ArvB a3;
  inicializa(&a3);
  for (v=1;v<=10; v++){
    r1.chave = v;
    insere(&a3, &r1);
  }
  exibeArvore(a3.raiz,1);
  printf("Altura da arvore: %i\n", altura(a3.raiz));

  return 0;
}