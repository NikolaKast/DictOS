.code16
.intel_syntax noprefix
.global _start
_start:
# Инициализация адресов сегментов. Эти операции требуется не для любого BIOS, но их рекомендуется проводить.
mov ax, cs # Сохранение адреса сегмента кода в ax
mov ds, ax # Сохранение этого адреса как начало сегмента данных
mov ss, ax # И сегмента стека
mov sp, _start # Сохранение адреса стека как адрес первой инструкции этого кода. Стек будет расти вверх и не перекроет код.
mov ah, 0x0e # 0x0e вывод символа на активную видео страницу (эмуляция телетайпа)

mov ax, 0x0003
int 0x10



mov ah, 0x13
mov al, 0x01
mov bh, 0x00
mov bl, 0x0C
mov cx, 37
mov bp, offset choose
int 0x10



mov bx, 26  # Отрисовка пустых значений
    output_loop:
    dec bx
    mov al, [buffer + bx]
    call putc
    cmp bx, 0
    jne output_loop

main_loop:

    call getc # Получение символа

    cmp al, '\r'    # Проверка нажатия энтер
    je test
    
    sub al, 97  # Проверка границ
    cmp al, 0
    jl main_loop
    cmp al, 26
    jg main_loop

    xor bx, bx 
    mov bl, al
    add al, 97 

    cmp [buffer + bx], al   # Проверка символ или _
    je symbolback
    mov [buffer + bx], al
    call changesymbol
    
    
    
jmp main_loop







# Функции
changesymbol:
    mov ah, 0x02 # постановка курсора на нужном символе
    mov bh, 0
    mov dh, 1     
    mov dl, bl   
    int 0x10

    mov ah, 0x09 # перезапись данного символа
    mov bh, 0
    mov bh, 0
    mov cx, 1
    mov bl, 0x0F
    int 0x10
    ret

symbolback:
    mov byte ptr [buffer + bx], 0x5F # Вовзрат _ если символ уже есть
    mov al, 0x5F
    call changesymbol
    jmp main_loop

buffer:
    .fill 26, 1, 0x5F



choose:
    .ascii "Press key of what words u wonna see\n\r"
    ret

getc:
    mov ah, 0x00
    int 0x16
    ret

putc:
    mov ah, 0x0e
    int 0x10
    ret





# Отключение прерываний
test:

mov si, offset buffer
mov di, 0x9000     # физический адрес 0x9000

mov cx, 26

copy_loop:
    mov al, [si]    # копирование только выбранных букв
    cmp al, 0x5F
    je dont_copy
    mov [di], al
    inc di
    dont_copy:
    inc si
    loop copy_loop



load_kernel:

    mov ax, 0x1000      # сегмент загрузки ядра
    mov es, ax

    xor bx, bx          # bx = 0

    mov ah, 0x02        # функция BIOS: чтение секторов
    mov al, 45          # количество секторов

    mov ch, 0x00        # цилиндр
    mov cl, 0x01        # первый сектор (нумерация с 1!)
    mov dh, 0x00        # головка
    mov dl, 0x01        # диск 1 (fdb)

    int 0x13            # вызов BIOS

    xor ebx, ebx        # ebx = 0
    xor ecx, ecx        # ecx = 0
    mov cl, 1           # cl = 1


 cli

# Загрузка размера и адреса таблицы дескрипторов
lgdt gdt_info

# Включение адресной линии A20
in al, 0x92
or al, 2
out 0x92, al

# Установка бита PE регистра CR0 - процессор перейдет в защищенный режим
mov eax, cr0
or eax, 1
mov cr0, eax



ljmp 0x8:protected_mode # "Дальний" переход для загрузки корректной информации в cs, архитектурные особенности не позволяют этого сделать напрямую

gdt:
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00
    .byte 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00

gdt_info:
    .word gdt_info - gdt
    .word gdt, 0

.code32
protected_mode: # Здесь идут первые инструкции в защищенном режиме
# Загрузка селекторов сегментов для стека и данных в регистры
mov ax, 0x10 # Используется дескриптор с номером 2 в GDT
mov es, ax
mov ds, ax
mov ss, ax
# Передача управления загруженному ядру
call 0x10000 # Адрес равен адресу загрузки в случае если ядро скомпилировано в "плоский" код
# Внимание! Сектор будет считаться загрузочным, если содержит в конце своих 512 байтов два следующих байта: 0x55 и 0xAA
.zero (512 - ($ - _start) - 2) # Заполнение нулями до границы 512 - 2 текущей точки 2 необходимых байта чтобы сектор считался загрузочным
.byte 0x55, 0xAA