//  PINOS DOS SENSORES (8 sensores IR)
const int SENSOR_PINS[8] = {34, 35, 32, 33, 25, 26, 27, 14};

//  PINOS DO MOTOR 1
#define MOTOR1_IN1   18   // Direção A
#define MOTOR1_IN2   19   // Direção B
#define MOTOR1_EN    21   // PWM Enable

//  PINOS DO MOTOR 2
#define MOTOR2_IN3   22   // Direção A
#define MOTOR2_IN4   23   // Direção B
#define MOTOR2_EN    16   // PWM Enable

//  CONFIGURAÇÃO DOS CANAIS PWM (LEDC)
#define PWM_CANAL_M1   0       // Canal LEDC para Motor 1
#define PWM_CANAL_M2   1       // Canal LEDC para Motor 2
#define PWM_FREQ       5000    // Frequência Hz
#define PWM_RESOLUCAO  8       // 8 bits → 0–255

#define VEL_BASE    180   // Velocidade base
#define VEL_MIN      60   // Velocidade mínima para as curvas
#define VEL_MAX     230   // Velocidade máxima


float Kp = 25.0;
float Ki = 0.0;
float Kd = 15.0;
float integral       = 0;

// Retorna array com 0 (branco) ou 1 (preto/linha)
void lerSensores(int leituras[8]) {
  for (int i = 0; i < 8; i++) {
    leituras[i] = digitalRead(SENSOR_PINS[i]) == LOW ? 1 : 0;//mudar paraa high a depender do sensor
  }
}

bool linhaEsquerda(int leituras[8]) {
  int esq = leituras[0] + leituras[1] + leituras[2] + leituras[3];
  int dir = leituras[4] + leituras[5] + leituras[6] + leituras[7];
  return esq > dir;
}

bool linhaDireita(int leituras[8]) {
  int esq = leituras[0] + leituras[1] + leituras[2] + leituras[3];
  int dir = leituras[4] + leituras[5] + leituras[6] + leituras[7];
  return dir > esq;
}

// Retorna true se linha está centrada
bool linhaCentro(int leituras[8]) {
  return !linhaEsquerda(leituras) && !linhaDireita(leituras);
}

//  pwm1 : potência motor 1 pwm2 : potência motor 2
//  Positivo = frente Negativo = ré
void girar(int pwm1, int pwm2) {

  //Motor 1
  pwm1 = constrain(pwm1, -255, 255);
  if (pwm1 > 0) {
    digitalWrite(MOTOR1_IN1, HIGH);
    digitalWrite(MOTOR1_IN2, LOW);
    ledcWrite(PWM_CANAL_M1, pwm1);
  } else if (pwm1 < 0) {
    digitalWrite(MOTOR1_IN1, LOW);
    digitalWrite(MOTOR1_IN2, HIGH);
    ledcWrite(PWM_CANAL_M1, -pwm1);
  } else {
    digitalWrite(MOTOR1_IN1, LOW);
    digitalWrite(MOTOR1_IN2, LOW);
    ledcWrite(PWM_CANAL_M1, 0);
  }

  //Motor 2
  pwm2 = constrain(pwm2, -255, 255);
  if (pwm2 > 0) {
    digitalWrite(MOTOR2_IN3, HIGH);
    digitalWrite(MOTOR2_IN4, LOW);
    ledcWrite(PWM_CANAL_M2, pwm2);
  } else if (pwm2 < 0) {
    digitalWrite(MOTOR2_IN3, LOW);
    digitalWrite(MOTOR2_IN4, HIGH);
    ledcWrite(PWM_CANAL_M2, -pwm2);
  } else {
    digitalWrite(MOTOR2_IN3, LOW);
    digitalWrite(MOTOR2_IN4, LOW);
    ledcWrite(PWM_CANAL_M2, 0);
  }
}

void parar() {
  girar(0, 0);
}

void frente(int vel) {
  girar(vel, vel);
}

void virarEsquerda(int vel_esq, int vel_dir) {
  girar(vel_esq, vel_dir);
}

void virarDireita(int vel_esq, int vel_dir) {
  girar(vel_esq, vel_dir);
}



void setup() {
  Serial.begin(115200);

  // Sensores como entrada
  for (int i = 0; i < 8; i++) {
    pinMode(SENSOR_PINS[i], INPUT);
  }

  // Pinos de direção como saída
  pinMode(MOTOR1_IN1, OUTPUT);
  pinMode(MOTOR1_IN2, OUTPUT);
  pinMode(MOTOR2_IN3, OUTPUT);
  pinMode(MOTOR2_IN4, OUTPUT);

  // Configura canais PWM (LEDC)
  ledcSetup(PWM_CANAL_M1, PWM_FREQ, PWM_RESOLUCAO);
  ledcSetup(PWM_CANAL_M2, PWM_FREQ, PWM_RESOLUCAO);
  ledcAttachPin(MOTOR1_EN, PWM_CANAL_M1);
  ledcAttachPin(MOTOR2_EN, PWM_CANAL_M2);

  parar();
  delay(2000); // Aguarda 2s antes de começar
  Serial.println("Seguidor de linha iniciado!");
}

void loop() {
  int leituras[8];
  lerSensores(leituras);

  // Debug
  Serial.print("Sensores: ");
  for (int i = 0; i < 8; i++) Serial.print(leituras[i]);
  Serial.print(" | ");
  //teste baixa vel/potencia para testar o codigo
  if (linhaCentro(leituras)) {
    frente(50);
  } else if (linhaDireita(leituras)) {
    virardireita(50,25);//valores teste
  } else{
    viraresquerda(50,25);//valores teste
  }


  // Calcula PWM de cada motor
  int pwm_esq = (int)(VEL_BASE - correcao);
  int pwm_dir = (int)(VEL_BASE + correcao);

  // Limita velocidades
  pwm_esq = constrain(pwm_esq, VEL_MIN, VEL_MAX);
  pwm_dir = constrain(pwm_dir, VEL_MIN, VEL_MAX);

  // Aplica nos motores
  girar(pwm_esq, pwm_dir);
  Serial.print(" | Correcao: "); Serial.print(correcao, 2);
  Serial.print(" | M1: "); Serial.print(pwm_esq);
  Serial.print(" | M2: "); Serial.println(pwm_dir);

  delay(10);
}
