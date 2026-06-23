#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/string.h>

#define DEVICE_NAME "telemetry"
#define COMMAND_SIZE 64
#define TELEMETRY_LINE_SIZE 64

/* Numero major recebido dinamicamente no module_init. */
static int major_number;

/* Usado apenas para dar um id simples a cada sessao aberta. */
static int next_session_id = 1;
static DEFINE_MUTEX(session_lock);

/* Configuracao global compartilhada entre todos os processos. */
static bool emergency_stop;
static DEFINE_MUTEX(config_lock);

/* Dados privados de cada open(). */
struct telemetry_session {
    int id;
    size_t bytes_read;
    size_t bytes_written;
};

static void trim_command(char *command)
{
    size_t len = strlen(command);

    while (len > 0) {
        char c = command[len - 1];

        if (c != '\n' && c != '\r' && c != ' ' && c != '\t')
            break;

        command[len - 1] = '\0';
        len--;
    }
}

/*
 * open:
 * Cria uma sessao para este descritor de arquivo e guarda em private_data.
 */
static int telemetry_open(struct inode *inode, struct file *file)
{
    struct telemetry_session *session;

    session = kzalloc(sizeof(*session), GFP_KERNEL);
    if (!session)
        return -ENOMEM;

    mutex_lock(&session_lock);
    session->id = next_session_id++;
    mutex_unlock(&session_lock);
    file->private_data = session;

    printk(KERN_INFO "telemetry: open sessao=%d\n", session->id);
    return 0;
}

/*
 * read:
 * Monta uma linha de telemetria em texto e copia para o espaco do usuario.
 */
static ssize_t telemetry_read(struct file *file, char __user *buffer,
                              size_t count, loff_t *ppos)
{
    struct telemetry_session *session = file->private_data;
    char line[TELEMETRY_LINE_SIZE];
    bool stop;
    int temperature;
    int pressure;
    int len;
    size_t line_len;
    size_t bytes_to_copy;

    if (!session)
        return -EIO;

    /* Depois da primeira leitura, retorna EOF para comandos como cat terminarem. */
    if (*ppos > 0)
        return 0;

    mutex_lock(&config_lock);
    stop = emergency_stop;
    mutex_unlock(&config_lock);

    temperature = stop ? 0 : 25;
    pressure = 101325;

    len = scnprintf(line, sizeof(line), "TEMP=%d.0;PRESSAO=%d;STATUS=%s\n",
                    temperature, pressure, stop ? "EMERGENCY" : "OK");

    line_len = len;
    bytes_to_copy = count < line_len ? count : line_len;

    if (copy_to_user(buffer, line, bytes_to_copy))
        return -EFAULT;

    session->bytes_read += bytes_to_copy;
    *ppos += bytes_to_copy;

    return bytes_to_copy;
}

/*
 * write:
 * Recebe comandos simples em texto e altera a configuracao global.
 */
static ssize_t telemetry_write(struct file *file, const char __user *buffer,
                               size_t count, loff_t *ppos)
{
    struct telemetry_session *session = file->private_data;
    char command[COMMAND_SIZE];

    if (!session)
        return -EIO;

    if (count == 0)
        return 0;

    if (count >= sizeof(command))
        return -EINVAL;

    if (copy_from_user(command, buffer, count))
        return -EFAULT;

    command[count] = '\0';
    trim_command(command);

    if (strcmp(command, "EMERGENCY_STOP=1") == 0) {
        mutex_lock(&config_lock);
        emergency_stop = true;
        mutex_unlock(&config_lock);
        printk(KERN_INFO "telemetry: emergencia ativada\n");
    } else if (strcmp(command, "EMERGENCY_STOP=0") == 0) {
        mutex_lock(&config_lock);
        emergency_stop = false;
        mutex_unlock(&config_lock);
        printk(KERN_INFO "telemetry: emergencia desativada\n");
    } else if (strcmp(command, "RESET") == 0) {
        mutex_lock(&config_lock);
        emergency_stop = false;
        mutex_unlock(&config_lock);
        printk(KERN_INFO "telemetry: reset aplicado\n");
    } else {
        printk(KERN_WARNING "telemetry: comando invalido: %s\n", command);
        return -EINVAL;
    }

    session->bytes_written += count;
    *ppos += count;

    return count;
}

/*
 * release:
 * Mostra estatisticas da sessao e libera a memoria alocada no open().
 */
static int telemetry_release(struct inode *inode, struct file *file)
{
    struct telemetry_session *session = file->private_data;

    if (session) {
        printk(KERN_INFO "telemetry: release sessao=%d lidos=%zu escritos=%zu\n",
               session->id, session->bytes_read, session->bytes_written);
        kfree(session);
        file->private_data = NULL;
    }

    return 0;
}

static const struct file_operations telemetry_fops = {
    .owner = THIS_MODULE,
    .open = telemetry_open,
    .read = telemetry_read,
    .write = telemetry_write,
    .release = telemetry_release,
};

/*
 * module_init:
 * Registra um driver de caractere simples e recebe um major dinamico.
 */
static int __init telemetry_init(void)
{
    major_number = register_chrdev(0, DEVICE_NAME, &telemetry_fops);
    if (major_number < 0) {
        printk(KERN_ALERT "telemetry: erro ao registrar driver\n");
        return major_number;
    }

    printk(KERN_INFO "telemetry: modulo carregado, major=%d\n", major_number);
    return 0;
}

/*
 * module_exit:
 * Remove o registro feito no module_init.
 */
static void __exit telemetry_exit(void)
{
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "telemetry: modulo removido\n");
}

module_init(telemetry_init);
module_exit(telemetry_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Francisco Eduardo de Araujo Sampaio");
MODULE_DESCRIPTION("Driver de caractere didatico para telemetria");
MODULE_VERSION("0.2");
