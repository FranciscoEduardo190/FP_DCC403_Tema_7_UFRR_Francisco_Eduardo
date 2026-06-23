# Driver de Telemetria - DCC403

**Autor:** Francisco Eduardo de Araújo Sampaio

Este trabalho foi desenvolvido para a disciplina de Sistemas Operacionais I, com o objetivo de demonstrar a implementação de um módulo simples de kernel Linux usando um driver de caractere. O projeto simula uma interface de telemetria, permitindo que processos em espaço de usuário leiam dados básicos e enviem comandos para alterar o estado interno do driver.

O módulo cria um dispositivo chamado `telemetry` e registra dinamicamente seu número major ao ser carregado. A partir das operações padrão de arquivo (`open`, `read`, `write` e `release`), o driver controla sessões individuais, contabiliza bytes lidos e escritos e mantém uma configuração global de parada de emergência.

## Resumo do Funcionamento

- `read`: retorna uma linha de telemetria com temperatura, pressão e status.
- `write`: aceita comandos simples para ativar, desativar ou resetar a parada de emergência.
- `open`: cria uma sessão privada para cada descritor de arquivo aberto.
- `release`: libera a sessão e registra informações básicas no log do kernel.

Comandos aceitos pelo driver:

```text
EMERGENCY_STOP=1
EMERGENCY_STOP=0
RESET
```

## Compilação

Para compilar o módulo, execute:

```sh
make
```

Para limpar os arquivos gerados:

```sh
make clean
```

## Ambiente de Teste QEMU

O módulo compilado incluído no projeto foi gerado para o kernel do ambiente QEMU, não para o kernel da máquina host. O kernel usado no QEMU é `6.17.9-76061709-generic`, e o arquivo `qemu_test/initramfs/root/telemetry_driver.ko` possui `vermagic=6.17.9-76061709-generic`. Por isso, em uma máquina host com outro kernel, como `6.18.7-76061807-generic`, o carregamento pode falhar por incompatibilidade de versão. Para testar o `.ko` incluído, utilize o ambiente disponível em `qemu_test/`.

## Observação

Este README apresenta apenas uma introdução e um resumo do projeto. A explicação completa, incluindo detalhes de implementação, testes e análise, está disponível no relatório colocado junto aos arquivos do trabalho.
