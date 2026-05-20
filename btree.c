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
  TipoChave chaves[2*T-1]; //fica fora da union, todos tem chaves (ou pra rotear ou guardar)

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



void exibeNo(No* atual){
  int x;
  printf("Chaves no No atual [%p]: %i\n", atual, atual->numChaves);
  for(x=0;x<atual->numChaves;x++){

    if (atual->folha) { //se o nó for folha, ele entra na parte das folhas info.folha, entra nos registros e busca a chavae
    printf("%i ", atual->info.folha.regs[x].chave);
    } else { // se for interno exibe a chave (sinalização)
        printf("%i ", atual->chaves[x]);
        }
    }
  printf("\n");
}

int altura(No* atual){
  if (atual->folha) return 0;
  int x, temp, max = 0;  
  for(x=0;x<=atual->numChaves;x++){

    temp = altura(atual->info.interno.filhos[x]); //o caminho pra acessar o filho mudou, antes era atual->filhos[x]
    if (temp>max) max = temp;
  }
  return max+1;
}


void exibeEnderecos(No* atual){
    //antes o código assumia q todo mundo tinha filho
  if (atual->folha) return;  
  int x;

  printf("Links no No atual [%p]: %i\n", atual, atual->numChaves+1);
  for(x=0;x<atual->numChaves+1;x++){
    //o caminho do filho mudou, igual a da funçao de cima
    printf("%p ", atual->info.interno.filhos[x]);
  }
  printf("\n");
}

void exibeArvore(No* atual, int nivel){
  printf("Nivel: %i - ", nivel);
  exibeNo(atual);

  if (atual->folha) return; //a condicao de folha já existia antes

  int i; //mesmo caminho do filho
  for (i=0;i<=atual->numChaves;i++) exibeArvore(atual->info.interno.filhos[i], nivel+1);
}


void salvarNoDisco(No* no){
  printf("O no com endereco %p seria salvo no disco.\n", no);
}

void lerDoDisco(No* no){
  printf("O no com endereco %p seria lido do disco.\n", no);
}



bool buscaRegistroAux(No* atual, TipoChave ch, Registro* reg){
    int i = 0;
    //se for folha a chave tá no registro
    if (atual->folha) {
      while (i < atual->numChaves && ch > atual->info.folha.regs[i].chave) i++;
	
    } else {//se for interno a chave tá no array
      while (i < atual->numChaves && ch > atual->chaves[i]) i++;
    }

    //se ele for folha, achou a chave, copia e retorna true
    if (atual->folha && i < atual->numChaves && ch == atual->info.folha.regs[i].chave){
      *reg = atual->info.folha.regs[i];
      return true;

    }else{ //se não achou e for interno, busca no próximo
      if(!atual->folha){
        return buscaRegistroAux(atual->info.interno.filhos[i], ch, reg);
      }
  }// se não achou em nenhum lugar retorna falso
  return false;
}

bool buscaRegistro(ArvB* a, TipoChave ch, Registro* reg){
  return buscaRegistroAux(a->raiz, ch, reg);
}


void divideNoFilho(No* x, int i, No* y){
  No* novo = (No*) malloc(sizeof(No));
  novo->folha = y->folha;
  int j;

  if (y->folha) {
      //o novo nó fica com T elementos
      novo->numChaves = T;
      for (j=0; j<T; j++) {
        // move um pra trás pra pegar o meio
        novo->info.folha.regs[j] = y->info.folha.regs[j+T-1];
      }
      //liga o ponteiro da lista quando a folha racha
      novo->info.folha.prox = y->info.folha.prox;
      y->info.folha.prox = novo;

  } else {
      //se for interno o tam fica igual da B e a chave sobe  
      novo->numChaves = T - 1;
      for (j=0; j<T-1; j++) novo->chaves[j] = y->chaves[j+T];
  }


  if (!y->folha)
    for (j=0; j<T; j++) novo->info.interno.filhos[j] = y->info.interno.filhos[j+T];
  y->numChaves = T - 1; // o nó da esquerda fica com T-1

  // o pai é interno, então ele não mexe nos registros
  for (j=x->numChaves; j>i; j--) x->info.interno.filhos[j+1] = x->info.interno.filhos[j];
  x->info.interno.filhos[i+1] = novo;
  for (j=x->numChaves-1; j>=i; j--) x->chaves[j+1] = x->chaves[j]; //abriu espaco na placa (guia)

  if (y->folha) {
    x->chaves[i] = y->info.folha.regs[T-1].chave; // se rachou folha, copia do registro
  } else {
    x->chaves[i] = y->chaves[T-1]; //se rachou nó interno, pega a chave na array fora do union
  }
  x->numChaves++;
  salvarNoDisco(y);
  salvarNoDisco(novo);
  salvarNoDisco(x);
}

void insereNoNaoCheio(No* atual, Registro* reg){
  int i = atual->numChaves-1;

  if (atual->folha){
            //mudou o caminho do registro
    while (i>=0 && reg->chave < atual->info.folha.regs[i].chave){
      atual->info.folha.regs[i+1] = atual->info.folha.regs[i];
      i--;
    }   // mudou o caminho do registro
    atual->info.folha.regs[i+1] = *reg;
    atual->numChaves++;
    salvarNoDisco(atual); 
  }else{
        // mudou o caminho da chave no nó interno
    while (i>=0 && reg->chave < atual->chaves[i]) i--;
    i++;
    // mudou o caminho do filho
    lerDoDisco(atual->info.interno.filhos[i]);
    //mudou o caminho do filho
    if(atual->info.interno.filhos[i]->numChaves==2*T-1){
        // mudou o caminho do filho
      divideNoFilho(atual, i, atual->info.interno.filhos[i]);
      // mudou o caminho da chave no nó interno
      if (reg->chave > atual->chaves[i]) i++;
    } //mudou o caminho da chave no nó intern
    insereNoNaoCheio(atual->info.interno.filhos[i], reg);
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
    novo->folha = false; // a nova raíz nunca é folha pq ela vai ter filho

    //muda o caminho do filho
    novo->info.interno.filhos[0] = r; // a raiz antiga vira o primeiro filho da raiz nova

    divideNoFilho(novo, 0, r);
    insereNoNaoCheio(novo, reg);
  }
}


void imprimirFolhasSequencial(ArvB* a) {
  No* atual = a->raiz;
  if (atual->numChaves == 0) return;

  //vai sempre pegar o primeiro filho, garantindo que vá ao filho mais a esquerda
  while (!atual->folha) atual = atual->info.interno.filhos[0];

  //percorrer a lista das folhas até o final
  while (atual != NULL) {
    for (int i = 0; i < atual->numChaves; i++) {
      printf("%i ", atual->info.folha.regs[i].chave);
    }
    atual = atual->info.folha.prox;
  }
  printf("\n");
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