// Эта инструкция обязательно должна быть первой, т.к. этот код компилируется в бинарный,
// и загрузчик передает управление по адресу первой инструкции бинарного образа ядра ОС.
__asm("jmp kmain");
#define VIDEO_BUF_PTR (0xb8000)
#define IDT_TYPE_INTR (0x0E)
#define IDT_TYPE_TRAP (0x0F)
// Селектор секции кода, установленный загрузчиком ОС
#define GDT_CS (0x8)
#define PIC1_PORT (0x20)
#define CURSOR_PORT (0x3D4)
#define VIDEO_WIDTH (80) // Ширина текстового экрана
#define SIZE_DICT (51)

#define LETTERS ((char*)0x9000)

// Словари
const char* english_dict[SIZE_DICT] = {
    "air",           // a
    "apple",         // a
    "bird",          // b
    "book",          // b
    "car",           // c
    "cat",           // c
    "dog",           // d
    "door",          // d
    "earth",         // e
    "egg",           // e
    "fire",          // f
    "fish",          // f
    "good",          // g
    "grass",         // g
    "hand",          // h
    "house",         // h
    "ice",           // i
    "iron",          // i
    "jazz",          // j
    "jump",          // j
    "key",           // k
    "king",          // k
    "lamp",          // l
    "love",          // l
    "milk",          // m
    "moon",          // m
    "night",         // n
    "nose",          // n
    "open",          // o
    "orange",        // o
    "paper",         // p
    "pen",           // p
    "queen",         // q
    "quiet",         // q
    "red",           // r
    "river",         // r
    "sky",           // s
    "sun",           // s
    "table",         // t
    "tree",          // t
    "umbrella",      // u
    "under",         // u
    "violin",        // v
    "voice",         // v
    "water",         // w
    "wind",          // w
    "xenon",         // x
    "xray",          // x
    "yellow",        // y
    "young",         // y
    "zebra"          // z
};

const char* spanish_dict[SIZE_DICT + 1] = {
    "aire",          // air
    "manzana",       // apple
    "pajaro",        // bird
    "libro",         // book
    "coche",         // car
    "gato",          // cat
    "perro",         // dog
    "puerta",        // door
    "tierra",        // earth
    "huevo",         // egg
    "fuego",         // fire
    "pez",           // fish
    "bueno",         // good
    "hierba",        // grass
    "mano",          // hand
    "casa",          // house
    "hielo",         // ice
    "hierro",        // iron
    "jazz",          // jazz
    "saltar",        // jump
    "llave",         // key
    "rey",           // king
    "lampara",       // lamp
    "amor",          // love
    "leche",         // milk
    "luna",          // moon
    "noche",         // night
    "nariz",         // nose
    "abierto",       // open
    "naranja",       // orange
    "papel",         // paper
    "boligrafo",     // pen
    "reina",         // queen
    "silencio",      // quiet
    "rojo",          // red
    "rio",           // river
    "cielo",         // sky
    "sol",           // sun
    "mesa",          // table
    "arbol",         // tree
    "paraguas",      // umbrella
    "bajo",          // under
    "violin",        // violin
    "voz",           // voice
    "agua",          // water
    "viento",        // wind
    "xenon",         // xenon
    "rayosx",        // xray
    "amarillo",      // yellow
    "joven",         // young
    "cebra"          // zebra
};




unsigned char scan_to_ascii[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' '
};


char command[41];
unsigned short numcom = 0;
typedef void (*intr_handler)();

// Структура описывает данные об обработчике прерывания
struct idt_entry
{
	unsigned short base_lo; // Младшие биты адреса обработчика
	unsigned short segm_sel; // Селектор сегмента кода
	unsigned char always0; // Этот байт всегда 0
	unsigned char flags; // Флаги тип. Флаги: P, DPL, Типы - это константы - IDT_TYPE...
	unsigned short base_hi; // Старшие биты адреса обработчика
} __attribute__((packed)); // Выравнивание запрещено

// Структура, адрес которой передается как аргумент команды lidt
struct idt_ptr
{
	unsigned short limit;
	unsigned int base;
} __attribute__((packed)); // Выравнивание запрещено

struct idt_entry g_idt[256]; // Реальная таблица IDT
struct idt_ptr g_idtp; // Описатель таблицы для команды lidt

void out_str(int color, const char* ptr, unsigned int strnum);

void default_intr_handler()
{
	asm("pusha");
	// Дефолтная обработка
	asm("popa; leave; iret");
}
typedef void (*intr_handler)();
void cursor_moveto(unsigned int strnum, unsigned int pos);
void intr_reg_handler(int num, unsigned short segm_sel, unsigned short flags, intr_handler hndlr)
{
	unsigned int hndlr_addr = (unsigned int)hndlr;
	g_idt[num].base_lo = (unsigned short)(hndlr_addr & 0xFFFF);
	g_idt[num].segm_sel = segm_sel;
	g_idt[num].always0 = 0;
	g_idt[num].flags = flags;
	g_idt[num].base_hi = (unsigned short)(hndlr_addr >> 16);
}
// Функция инициализации системы прерываний заполнение массива с адресами обработчиков
void intr_init()
{
	int i;
	int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
	for (i = 0; i < idt_count; i++)
		intr_reg_handler(i, GDT_CS, 0x80 | IDT_TYPE_INTR,
			default_intr_handler); // segm_sel=0x8, P=1, DPL=0, Type=Intr
}
void intr_start()
{
	int idt_count = sizeof(g_idt) / sizeof(g_idt[0]);
	g_idtp.base = (unsigned int)(&g_idt[0]);
	g_idtp.limit = (sizeof(struct idt_entry) * idt_count) - 1;
	asm("lidt %0" : : "m" (g_idtp));
}
void intr_enable()
{
	asm("sti");
}
void intr_disable()
{
	asm("cli");
}

static inline unsigned char inb(unsigned short port) // Чтение из порта
{
    unsigned char data;
    asm volatile ("inb %w1, %b0" : "=a" (data) : "Nd" (port));
    return data;
}
static inline void outb(unsigned short port, unsigned char data) // Запись
{
    asm volatile ("outb %b0, %w1" : : "a" (data), "Nd" (port));
}

void clear_screen() {
    unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;


    for (int row = 1; row < 25; row++) {
        int offset = row * VIDEO_WIDTH * 2;

        for (int col = 0; col < VIDEO_WIDTH; col++) {
            video_buf[offset + col*2] = ' ';
            video_buf[offset + col*2 + 1] = 0x07;
        }
    }
    cursor_moveto(1, 0);
}

unsigned int strcmp(const char* ptr1, const char* ptr2){
	int i = 0;
	while(ptr1[i] != '\0' || ptr2[i] != '\0'){
	if(ptr1[i] != ptr2[i]){
		return 1;
	}
	i++;
}
if(ptr1[i] != ptr2[i]){
		return 1;
	}
else{
	return 0;
}
}



void inform(){
	const char* autor = "Developer - Galko Nikolay, student SPBPU from group 5131001/40002";
	const char* system = "The bootloder maked in GNU with Intel syntax, kernel maked on C with GCC";
	const char* input = "The letters what used in this dictionary:";
	out_str(0x07, autor, 3);
	out_str(0x07, system, 4);
	out_str(0x07, input, 5);
	out_str(0x07, LETTERS, 6);
}
char int_in_char(int num){
	if (num >= 0 && num <= 9) {
        return '0' + num;
    }
	else{
		return '\0';
	}
}

void printnum(int num, int str, int pos){
	char output[40];
	char padding[256];
	for(int j = 0;j < 256;j++){
		padding[j] = '\0';
	}
	int i = 10000;
	short size = 0;
	short flag = 0;
	int paddplus = 0;
	while(i != 1){
		if(num / i != 0 || flag > 0){
			flag++;
			output[size] = int_in_char(num / i);
			size++;
			num = num % i;
		}
		i = i / 10;
	}
	if(num / i != 0 || flag > 0 || num == 0){
			flag++;
			output[size] = int_in_char(num / i);
			size++;
		}
	if(flag > 0){
		int to_pos = 0;
		while(to_pos != pos + flag){
			if(to_pos < pos){
				padding[to_pos] = ' ';
			}
			else{
				padding[to_pos] = output[paddplus];
				paddplus++;
			}
			to_pos++;
		}
		out_str(0x07, padding, str);
	}
}
int now_dict(){
	int sum = 0;
	int i = 0;
	while(LETTERS[i] != '\0'){
		for(int j = 0; j < SIZE_DICT; j++){
			if(english_dict[j][0] == LETTERS[i]){
				sum++;
			}
		}
		i++;
	}
	return sum;
}

void dictinfo(){
	const char* language = "Dictionary en -> es";
	const char* allsize = "Full dictionary size: ";
	const char* nowsize = "Now dictionary size: ";
	out_str(0x07, language, 3);
	printnum(SIZE_DICT, 4, 22);
	out_str(0x07, allsize, 4);
	int now = now_dict();
	printnum(now, 5, 22);
	out_str(0x07, nowsize, 5);
}

char* getarg(){
	int i = 0;
	int flag = 0;
	static char arg[41];
	while(command[i] != ' ' && command[i] != '\0'){
		i++;
		if(command[i] == ' '){
			flag = 1;
		}
	}
	if(flag == 1){
		command[i] = '\0';
		i++;
		int j = 0;
		while(command[i] != '\0'){
			arg[j] = command[i];
			j++;
			i++;
		}
		arg[j] = '\0';
		return arg;
	}
	else{
		arg[0] = '\0';
		return arg;
	}
}

void wordstat(char word){
	int sum = 0;
	for(int j = 0; j < SIZE_DICT; j++){
			if(english_dict[j][0] == word){
				sum++;
		}
	}
	printnum(sum, 3, 22);
	const char* nullletter = "Words on letter ( ): ";
	out_str(0x07, nullletter, 3);
	char letter[20] = "                  ";
	letter[17] = word;
	out_str(0x07, letter, 3);
	const char* afterletter = "Words on letter (";
	out_str(0x07, afterletter, 3);
}
int check_acces(char word){
	int i = 0;
	int flag = 0;
	while(LETTERS[i] != '\0'){
		if(word == LETTERS[i]){
			flag = 1;
			return 0;
		}
		i++;
	}
	return 1;

}

int bin_search(char* arg) {
    int left = 0;
    int right = SIZE_DICT - 1;

    while (left <= right) {
        int mid = (left + right) / 2;

        int i = 0;
        int cmp_result = 0;

        while (1) {

            if (arg[i] == '\0' && english_dict[mid][i] == '\0') {
                cmp_result = 0;
                break;
            }

            if (arg[i] == '\0') {
                cmp_result = -1;
                break;
            }

            if (english_dict[mid][i] == '\0') {
                cmp_result = 1;
                break;
            }

            if (arg[i] < english_dict[mid][i]) {
                cmp_result = -1;
                break;
            }
            if (arg[i] > english_dict[mid][i]) {
                cmp_result = 1;
                break;
            }

            i++;
        }

        if (cmp_result == 0) {
            return mid;
        }
        else if (cmp_result > 0) {
            left = mid + 1;
        }
        else {
            right = mid - 1;
        }
    }
    return -1;
}

void translate(char* arg){
	int result = check_acces(arg[0]);
	if (result == 0){

		int index = bin_search(arg);
		if(index == -1){
			const char* error = "Incorrect argument or no word in dictionary:";
			const char* again = "Try another word again";
			out_str(0x07, error, 3);
			out_str(0x07, arg, 4);
			out_str(0x07, arg, 5);

		}
		else{
				out_str(0x07, english_dict[index], 3);
				const char* strel = "->";
				out_str(0x07, strel, 4);
				out_str(0x07, spanish_dict[index], 5);
		}

	}
	else{
		char letter[32] = "                              ";
		letter[30] = arg[0];
		out_str(0x07, letter, 3);
		const char* error = "You didnt choose that letter: ";
		out_str(0x07, error, 3);
	}
}


void check_command(){
	const char* info = "info";
	int result = strcmp(info, command);
	if(result == 0){
		inform();
		return;
	}

	const char* dictinf = "dictinfo";
	result = strcmp(dictinf, command);
	if(result == 0){
		dictinfo();
		return;
	}

	const char* shut = "shutdown";
	result = strcmp(shut, command);
	if(result == 0){
		outb(0x64, 0xFE);
		return;
	}

	char* arg = getarg();

	const char* words = "wordstat";
	result = strcmp(words,command);
	if(result == 0){
		if(arg[0] != '\0' && arg[1] == '\0' && (arg[0] >= 'a' && arg[0] <= 'z')){
			wordstat(arg[0]);
		}
		else{
			const char* wrongarg = "Unknown argument:";
			const char* again = "Use one letter";
			out_str(0x07, wrongarg, 3);
			out_str(0x07, arg, 4);
			out_str(0x07, again, 5);
		}
		return;
	}
	const char* trans = "translate";
	result = strcmp(trans,command);
	if(result == 0){
		if(arg[0] != '\0' && (arg[0] >= 'a' && arg[0] <= 'z')){
			translate(arg);
		}
		else{
			const char* wrongarg = "Unknown argument:";
			const char* again = "Use one word";
			out_str(0x07, wrongarg, 3);
			out_str(0x07, arg, 4);
			out_str(0x07, again, 5);
		}
		return;
	}

	const char* wrongcommand = "Unknown command:";
	const char* again = "Try again, u can use commands:";
	out_str(0x07, wrongcommand, 3);
	out_str(0x07, command, 4);
	out_str(0x07, again, 5);
	out_str(0x07, info, 6);
	out_str(0x07, dictinf, 7);
	out_str(0x07, shut, 8);
	out_str(0x07, words, 9);
	out_str(0x07, trans, 10);


}




void on_key(unsigned char scan_code) {
	if(numcom != 40){
		clear_screen();
	}
	if(scan_code == 0x1C){ // Enter
		check_command();
		while(numcom != 0){
			command[numcom] = ' ';
			out_str(0x07, command, 1);
			command[numcom] = '\0';
			numcom--;
		}
		command[numcom] = ' ';
		out_str(0x07, command, 1);
		command[numcom] = '\0';
		cursor_moveto(1, numcom);
	}
	else if(scan_code == 0x0E && numcom > 0){
		command[numcom-1] = ' ';
		numcom--;
		cursor_moveto(1, numcom);
		out_str(0x07, command, 1);
		command[numcom + 1] = '\0';
	}
    else if (scan_code < sizeof(scan_to_ascii) && scan_code != 0x0E && numcom < 40) {


        command[numcom] = scan_to_ascii[scan_code];
        if (command[numcom] != 0) {
            out_str(0x07, command, 1);
			cursor_moveto(1, numcom+1);
			command[numcom + 1] = '\0';
			numcom++;
        }


	}
}

void keyb_process_keys()
{
    // Проверка что буфер PS/2 клавиатуры не пуст (младший бит присутствует)
    if (inb(0x64) & 0x01)
    {
        unsigned char scan_code;
        unsigned char state;
        scan_code = inb(0x60); // Считывание символа с PS/2 клавиатуры
        if (scan_code < 128) // Скан-коды выше 128 - это отпускание клавиши
            on_key(scan_code);
    }
}

void keyb_handler()
{
    asm("pusha");
    // Обработка поступивших данных
    keyb_process_keys();
    // Отправка контроллеру 8259 нотификации о том, что прерывани обработано
        outb(PIC1_PORT, 0x20);
    asm("popa; leave; iret");
}

void keyb_init()
{
    intr_reg_handler(0x09, GDT_CS, 0x80 | IDT_TYPE_INTR, keyb_handler);
    // segm_sel=0x8, P=1, DPL=0, Type=Intr
    // Разрешение только прерываний клавиатуры от контроллера 8259
    outb(PIC1_PORT + 1, 0xFF ^ 0x02); // 0xFF - все прерывания, 0x02 - бит IRQ1(клавиатура).
        // Разрешены будут только прерывания, чьи биты установлены в 0
}

void cursor_moveto(unsigned int strnum, unsigned int pos)
{

    unsigned short new_pos = (strnum * VIDEO_WIDTH) + pos;
    outb(CURSOR_PORT, 0x0F);
    outb(CURSOR_PORT + 1, (unsigned char)(new_pos & 0xFF));
    outb(CURSOR_PORT, 0x0E);
    outb(CURSOR_PORT + 1, (unsigned char)((new_pos >> 8) & 0xFF));
}

void out_str(int color, const char* ptr, unsigned int strnum)
{
    unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
    video_buf += 80 * 2 * strnum;
    while (*ptr)
    {
        video_buf[0] = (unsigned char)*ptr;
        video_buf[1] = color;
        video_buf += 2;
        ptr++;
    }
}


extern "C" int kmain()
{
	unsigned char* video_buf = (unsigned char*)VIDEO_BUF_PTR;
	for (int i = 0; i < VIDEO_WIDTH * 25; i++)
		*(video_buf + i * 2) = '\0';
	const char* hello = "Welcome to DictOS (gcc edition)!";
	out_str(0x07, hello, 0);
	intr_init();
	keyb_init();
	intr_start();
	intr_enable();
	cursor_moveto(1,0);
	while (1)
	{
		asm("hlt");
	}
	return 0;
}

