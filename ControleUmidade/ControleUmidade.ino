#include <LiquidCrystal.h>

// Constantes do circuito
const int PINO_TERMISTOR  = A0;
const int PINO_RELE       = 6;
const int PINO_SELECAO    = 10;
const int PINO_BOTAO_UP   = 9;
const int PINO_BOTAO_DOWN = 8;
const double CORRENTE_DO_CIRCUITO = 5.0;
const double RESISTOR_DO_CIRCUITO = 10000.0;
const int QUANTIDADE_DE_MEDICAO   = 10;
const int CICLO    = 1000; //1s
const int LIGAR    = 0;
const int DESLIGAR = 1;
const int TEMPERATURA_MAXIMA = 70;
const int TEMPERATURA_MINIMA = 45;
const int TEMPERATURA_DE_OSCILACAO = 2;
const int TEMPERATURA_DE_AJUSTE    = 5;
const int TEMPERATURA_DE_FOLGA     = 1;
const int TEMPO_DE_FUNCIONAMENTO_AJUSTE = 1;
const int OPCAO_TEMPERATURA = 0;
const int OPCAO_TEMPO       = 1;

//-Parâmetros do termistor
const double BETA = 3600.0;
const double R0 = 10000.0;
const double T0 = 273.0 + 25.0;
const double RX = R0 * exp(-BETA/T0);

//Globais
int _tempoEstimado = 0;
int _tempoDecorrido = 0;
int _tempoCronometrado = 0;
int _temperaturaBase = TEMPERATURA_MINIMA;
int _temperaturaInterna = 0;
int _umidadeInterna = 0;
int _opcaoSelecionada = 0; //0 = Temperatura Base | 1 = Tempo de Funcionamento
bool _iniciar = false;
bool _aquecendo = false;
bool _protecao = false;

//Inicializacao
LiquidCrystal lcd(13,12,5,4,3,2);

// Iniciação
void setup() {

  lcd.begin(16,2);
  
  pinMode(PINO_RELE,OUTPUT);
  pinMode(PINO_BOTAO_UP,INPUT_PULLUP);
  pinMode(PINO_BOTAO_DOWN,INPUT_PULLUP);
  pinMode(PINO_SELECAO, INPUT_PULLUP);

  Serial.begin(115200);
}
 
void loop() {
  
  MediaDaTemperatura(RangeDeTemperatura(PINO_TERMISTOR,QUANTIDADE_DE_MEDICAO));
  VerificarProtecaoDaEstufa();
      
  if(!_protecao)
  {
    _umidadeInterna = 00;
    AjustarOpcoes();
    LigarAquecimento();
    EscreverNoVisor();
  }
  
  delay (CICLO); // Dá um tempo entre leituras
}

void ContarTempo()
{
  if(_iniciar)
  {
    if(_tempoCronometrado == 3200)
    {
      _tempoDecorrido ++;
      _tempoCronometrado = 0;
    }
    
    _tempoCronometrado ++;
    
  }else
  {
    _tempoDecorrido = 0;
    _tempoEstimado = 0;
    _tempoCronometrado = 0;
  }
}

//Metodos Principais - Inicio
int RangeDeTemperatura(int pino, int quantidade)
{
  int soma = 0;
  for (int i = 0; i < quantidade; i++) {
    soma += analogRead(pino);
    delay (10);
  }
  
  return soma;
}

int MediaDaTemperatura(int rangeDeTemperatura)
{
  int temperatura = rangeDeTemperatura;
  if (!temperatura == 0)
  {
    double resistenciaDoTermistor = GetResistenciaTermistor(rangeDeTemperatura);
    temperatura = (GetTemperaturaAtual(resistenciaDoTermistor)-273.0);
  }
  _temperaturaInterna = temperatura;
}

void VerificarProtecaoDaEstufa()
{
  if(_temperaturaInterna == 0 || _temperaturaInterna > (TEMPERATURA_MAXIMA + TEMPERATURA_DE_FOLGA))
  {
    AquecerEstufa(DESLIGAR);
    _protecao = true;
    _iniciar = false;
    
    if (_temperaturaInterna > (TEMPERATURA_MAXIMA + TEMPERATURA_DE_FOLGA))
    {
      EscreverErro("Temperatura ALTA");
    }else
    {
      EscreverErro("Temperatura ZERO");
    }
  }  
  
}

void AjustarOpcoes()
{
  AjustarOpcaoSelecionada();
  
  if (digitalRead(PINO_BOTAO_UP) == LOW || digitalRead(PINO_BOTAO_DOWN) == LOW)
 {
   if (digitalRead(PINO_BOTAO_UP) == LOW)
   {
     AumentarValor();
   }else
   {
     DiminuirValor();
   }
 } 
}

void EscreverNoVisor()
{
  lcd.clear();

  EscreverTemperatura();
  EscreverUmidade();
  EscreverOpcoes();
}

void LigarAquecimento()
{
  if((_temperaturaInterna == (_temperaturaBase + TEMPERATURA_DE_FOLGA) || !ValidarTempoAquecimento()) && _aquecendo)
  {
    AquecerEstufa(DESLIGAR);
  }else
  
    if(_temperaturaInterna < ((_temperaturaBase + TEMPERATURA_DE_FOLGA) - TEMPERATURA_DE_OSCILACAO) && ValidarTempoAquecimento() && !_aquecendo)
    {
        AquecerEstufa(LIGAR);
    }
}

//Metodos Principais - Fim

bool ValidarTempoAquecimento()
{
  bool validado = false;
  
  if (_tempoDecorrido < _tempoEstimado)
  {
    _iniciar = true;
    validado = true;
  }else
  {
      _iniciar = false;
  }
  
  ContarTempo();
  
  return validado;
}

double GetResistenciaTermistor(int rangeTemperatura)
{
  double v = (CORRENTE_DO_CIRCUITO * rangeTemperatura)/(QUANTIDADE_DE_MEDICAO * 1024.0);
  return (CORRENTE_DO_CIRCUITO * RESISTOR_DO_CIRCUITO) / v - RESISTOR_DO_CIRCUITO;  
}

double GetTemperaturaAtual(double resistenciaTermistor)
{
  return (BETA / log(resistenciaTermistor / RX));
}

void AquecerEstufa(int acao)
{
  if (acao==0 && !_protecao)
  {
    _aquecendo = true;
    digitalWrite(PINO_RELE,HIGH);
  }else
  {
    _aquecendo = false;
    digitalWrite(PINO_RELE,LOW);
  }
 
}

//Metodo dos Botoes - Inicio

void AjustarOpcaoSelecionada()
{
  Serial.println("AjustarOpcaoSelecionada - Inicio");
  Serial.println(_opcaoSelecionada);
  
  if(digitalRead(PINO_SELECAO) == LOW)
  {
    Serial.println(digitalRead(PINO_SELECAO));
     if (_opcaoSelecionada == OPCAO_TEMPERATURA)
     {
       Serial.println("TEMPO");
       _opcaoSelecionada = OPCAO_TEMPO;
     }else
     {
       Serial.println("TEMPERATURA");
       _opcaoSelecionada = OPCAO_TEMPERATURA;
     }
  }  
  Serial.println("AjustarOpcaoSelecionada - FIM");
}

void AumentarValor()
{
  if(_opcaoSelecionada == OPCAO_TEMPERATURA)
  {
    AumentarTemperaturaBase();
  }else
  {
    AumentarTempoEstimado();
  }
}

void DiminuirValor()
{
  if(_opcaoSelecionada == OPCAO_TEMPERATURA)
  {
    DiminuirTemperaturaBase();
  }else
  {
    DiminuirTempoEstimado();
  }
}

void AumentarTempoEstimado()
{
  if(_tempoEstimado < 100)
  {
    _tempoEstimado = _tempoEstimado + TEMPO_DE_FUNCIONAMENTO_AJUSTE;
  }
}

void DiminuirTempoEstimado()
{
  if(_tempoEstimado > 0)
  {
    _tempoEstimado = _tempoEstimado - TEMPO_DE_FUNCIONAMENTO_AJUSTE;
  }
}

void AumentarTemperaturaBase()
{
  if(_temperaturaBase < TEMPERATURA_MAXIMA)
  {
    _temperaturaBase = _temperaturaBase + TEMPERATURA_DE_AJUSTE;
  }
}

void DiminuirTemperaturaBase()
{
  if(_temperaturaBase > TEMPERATURA_MINIMA)
  {
    _temperaturaBase = _temperaturaBase - TEMPERATURA_DE_AJUSTE;
  }
}


//Visor

void EscreverTemperatura()
{
  lcd.setCursor(0,0);
  lcd.print("T.In:");
  lcd.setCursor(5,0);
  lcd.print((String)_temperaturaInterna + "C");  
}

void EscreverUmidade()
{
  lcd.setCursor(9,0);
  lcd.print("Umi:");
  lcd.setCursor(13,0);
  lcd.print((String)_umidadeInterna + "%");
}

void EscreverTemperaturaBase()
{
  lcd.setCursor(1,1);
  lcd.print("T.P:");
  lcd.setCursor(5,1);
  lcd.print((String)_temperaturaBase + "C");
}

void EscreverTempo()
{
  lcd.setCursor(9,1);
  lcd.print("H:");
  lcd.setCursor(11,1);
  lcd.print(FormatarEscrita(_tempoDecorrido));
  lcd.setCursor(13,1);
  lcd.print("/");
  lcd.setCursor(14,1);
  lcd.print(FormatarEscrita(_tempoEstimado));
}

void EscreverOpcoes()
{
  
  EscreverTemperaturaBase();
  EscreverTempo();
  EscreverOpcaoSelecionada();
}

void EscreverOpcaoSelecionada()
{
  if(_opcaoSelecionada == OPCAO_TEMPERATURA)
  {
    lcd.setCursor(8,1);
    lcd.print("");
    lcd.setCursor(0,1);
    lcd.print(">");
  }else
  {
    lcd.setCursor(0,1);
    lcd.print("");
    lcd.setCursor(8,1);
    lcd.print(">"); 
  }
}

void EscreverErro(String erro)
{
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print("ERRO");
  lcd.setCursor(0,1);
  lcd.print(erro);
}

String FormatarEscrita(int valor)
{
  if (valor < 10)
  {
    return "0" + (String)valor;
  }
  
  return (String)valor;
}
