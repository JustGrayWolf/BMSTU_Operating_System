.586p 
    

descr struct	
	limit 	DW 0 
	base_l 	DW 0  
	base_m 	DB 0 
	attr_1 	DB 0
	attr_2 	DB 0 
	base_h 	DB 0
descr ends

idescr struc
    offs_l  dw 0 ; Смещение
    sel     dw 0 ; Селектор
    cntr    db 0 ; Счетчик.    
    attr    db 0 ; Байт атрибутов
    offs_h  dw 0 ; Смещение
idescr ends

; Сегмент стека.
stack32 segment  para stack 'STACK'
    stack_start db  100h dup(?)
    stack_size = $-stack_start
stack32 ends

; Сегмент данных
data32 segment para 'data'
    gdt_null  descr <>
    gdt_code16 descr <code16_size-1,0,0,98h>; p-присутствие сегмента в памяти, уровень привилегий , 1, тип сегмента,  Atach
    gdt_data4gb descr <0FFFFh,0,0,92h,0CFh> 
    
    gdt_code32 descr <code32_size-1,0,0,98h,40h>
    gdt_data32 descr <data_size-1,0,0,92h,40h>
    gdt_stack32 descr <stack_size-1,0,0,92h,40h>

	;
    gdt_video16 descr <3999,8000h,0Bh,92h> 

    gdt_size=$-gdt_null
   
    pdescr    df 0

    ; Селекторы - номер дескриптора в GDT.
    code16s=8
    data4gbs=16
    code32s=24
    data32s=32
    stack32s=40
    video16s=48

    ; idt - метка начала IDT
    idt label byte

    idescr_0_12 idescr 13 dup (<0,code32s,0,8Fh,0>) 
    
    idescr_13 idescr <0,code32s,0,8Fh,0>
    
    idescr_14_31 idescr 18 dup (<0,code32s,0,8Fh,0>)

    int08 idescr <0,code32s,0,10001110b,0> 
    int09 idescr <0,code32s,0,10001110b,0>

    ; Размер таблицы дескрипторов прерываний.
    idt_size=$-idt

    ipdescr df 0
	
    ipdescr16 dw 3FFh, 0, 0 

    mask_master db 0        
    mask_slave  db 0        
    time_ dw 0
    asciimap   db 0, 0, 49, 50, 51, 52, 53, 54, 55, 56, 57, 48, 45, 61, 0, 0
    db 81, 87, 69, 82, 84, 89, 85, 73, 79, 80, 91, 93, 0, 0, 65, 83
    db 68, 70, 71, 72, 74, 75, 76, 59, 39, 96, 0, 92, 90, 88, 67
    db 86, 66, 78, 77, 44, 46, 47,32,32,32,32,32,32,32,32,32

    flag_enter_pr db 0
    cnt_time      db 0            

    syml_pos      dd 2 * 80 * 5

    mem_pos=0 
    ; позиция на экране значения кол-ва доступной памяти 
    mem_value_pos=14 + 16 
    
    mb_pos=30 + 2
    cursor_pos=80 
    param=2Ah

    cursor_symb=219
    param_int_8 db 00Fh ; Цвет курсора.
    
    rm_msg      db 27, '[30;42mNow in Real Mode. ', 27, '[0m$', '$'
    pm_msg_wait db 27, '[30;42mPress any button to enter protected mode!', 27, '[0m$'
    pm_msg_out  db 27, '[30;42mNow in Real Mode again! ', 27, '[0m$'
    pm_mem_count db 'Memory: '

    data_size = $-gdt_null 
data32 ends


code32 segment para public 'code' use32
    assume cs:code32, ds:data32, ss:stack32

pm_start:
    mov ax, data32s
    mov ds, ax
    mov ax, video16s
    mov es, ax
    mov ax, stack32s
    mov ss, ax
    mov eax, stack_size
    mov esp, eax

    sti ; Резрешаем (аппаратные) прерывания
    
    ; Вывод сообщения "Memory"
    mov di, mem_pos
    mov ah, param
    xor esi, esi
    xor ecx, ecx
    mov cx, 8 
    print_memory_msg:
        mov al, pm_mem_count[esi]
        stosw 
        inc esi
    loop print_memory_msg

    ; Считаем и выводим кол-во физической памяти
    call count_memory_proc
    
    ; Цикл ожидание
    proccess:
        test flag_enter_pr, 1 ; если flag = 1, то выход
    jz  proccess
    
    cli ; Запрет аппаратных маскируемые прерывания прерываний.

    db  0EAh ; jmp
    dd  offset return_rm ; offset
    dw  code16s ; selector


    ; Зашлушка для исключений.
    except_1 proc
        iret
    except_1 endp

    ; Заглушка для 13 исключения.
    ; Нужно снять со стека код ошибки.
    except_13 proc uses eax
        pop eax
        iret
    except_13 endp

	print_int proc
	lable1:
	xor dx,dx
	mov bx, 10
	div bx
	add dl, '0'
	mov dh, param
	mov es:[edi], dx
	sub edi, 2
	cmp ax, 0
	jne lable1
	ret
	print_int endp

    new_int08 proc uses eax 
		mov ax, ds:time_
		inc ax 
		mov ds:time_, ax; увеличиваем счётчик
		mov edi, 90
		call print_int; вывод счётчика
        mov edi, cursor_pos ; поместим в edi позицию для вывода

        mov ah, param_int_8 ; В ah помещаем цвет текста.
        ror ah, 1           ; Сдвигаем циклически вправо параметр
        mov param_int_8, ah
        mov al, cursor_symb ; Символ, который мы хотим вывести (в моем случае просто квадрат).
        stosw ; al (символ) с параметром (ah) перемещается в область памяти es:di


        ; разрешаем обработку прерываний с меньшим приоритетом
        mov al, 20h
        out 20h, al

        iretd 
    new_int08 endp

    new_int09 proc uses eax ebx edx
        in  al, 60h ; считываем порт клавы
		cmp al, 0Eh ;сравнивем с backspace
		jne nextcmp ;удаление символа
		xor dx, dx
		mov ebx, syml_pos
		sub ebx, 2   
        mov es:[ebx], dx       
        mov syml_pos, ebx
		jmp exit
		nextcmp:
        cmp al, 1Ch ; сравниваем с Enter'ом

        jne print_value         
        or flag_enter_pr, 1 ; если Enter, устанавливаем флаг
        jmp exit

    print_value:
        ; это условие проверяет, отпущена ли была клавиша
        cmp al, 80h
        ja exit     

        xor ah, ah   

        xor ebx, ebx
        mov bx, ax

        mov dh, param
        mov dl, asciimap[ebx]   
        mov ebx, syml_pos   
        mov es:[ebx], dx

        add ebx, 2          
        mov syml_pos, ebx

    exit: 

        ; разрешаем обработку прерываний с меньшим приоритетом
        mov al, 20h 
        out 20h, al

        iretd
    new_int09 endp


    
    
    count_memory_proc proc uses ds eax ebx
        mov ax, data4gbs 
        mov ds, ax 
        mov ebx,  100001h ;пропускаем первый мегабайт
        mov dl,   0AEh ; Сигнатура, с помощью которого мы будем проверять запись.

        mov ecx, 0FFEFFFFEh


        count_memory:
            ; Сохраняем байт в dh.
            mov dh, ds:[ebx] 
            ; Записываем по этому адресу сигнатуру.
            mov ds:[ebx], dl        
            ; Сравниваем записанную сигнатуру с сигнатурой в программе.
            cmp ds:[ebx], dl        
        
            ; Если не равны, то это уже не наша память.
            jne print_memory_counter        
        
            mov ds:[ebx], dh    ; Обратно запиываем считанное значени.
            inc ebx             ; Увеличиваем счетчик.
        loop count_memory

    print_memory_counter:
        mov eax, ebx 
        xor edx, edx

        ; Мы считали по байту. Переводим в мегабайты.
        ; Деедим на 2^20 (кол-во байт в мегабайте).
        mov ebx, 100000h 
        div ebx

        mov ebx, mem_value_pos
        ; функция, которая печатает eax 
		mov edi, mb_pos
		sub edi, 6
        call print_int

        ; Печать надписи Mb (мегабайты)
        mov ah, param
        mov ebx, mb_pos
        mov al, 'M'
        mov es:[ebx], ax

        mov ebx, mb_pos + 2
        mov al, 'b'
        mov es:[ebx], ax
        ret

    count_memory_proc endp

    code32_size = $-pm_start
code32 ends


code16 segment para public 'CODE' use16
assume cs:code16, ds:data32, ss: stack32

NewLine: 
    ; Перенес на новую строку.
    xor dx, dx
    mov ah, 2   
    mov dl, 13 
    int 21h    
    mov dl, 10
    int 21h
    ret

ClearScreen:
    ; Инструкция очистки экрана
    mov ax, 3
    int 10h
    ret

start:
    mov ax, data32
    mov ds, ax

    ; Вывдим сообщение, о том, что мы в реальном режиме.
    mov ah, 09h
    lea dx, rm_msg
    int 21h
    call NewLine

    ; Вывод сообщения, что мы ожидаем нажатие клавиши. 
    mov ah, 09h
    lea dx, pm_msg_wait
    int 21h
    call NewLine

    ; Ожидание нажатия кнопки
    mov ah, 10h
    int 16h
    
    call ClearScreen

    xor eax, eax

    mov ax, code16
    shl eax, 4                  
    mov word ptr gdt_code16.base_l, ax  
    shr eax, 16                       
    mov byte ptr gdt_code16.base_m, al  
    mov byte ptr gdt_code16.base_h, ah  

    mov ax, code32
    shl eax, 4                        
    mov word ptr gdt_code32.base_l, ax  
    shr eax, 16                       
    mov byte ptr gdt_code32.base_m, al  
    mov byte ptr gdt_code32.base_h, ah  

    mov ax, data32
    shl eax, 4                        
    mov word ptr gdt_data32.base_l, ax  
    shr eax, 16                       
    mov byte ptr gdt_data32.base_m, al  
    mov byte ptr gdt_data32.base_h, ah  

    mov ax, stack32
    shl eax, 4                        
    mov word ptr gdt_stack32.base_l, ax  
    shr eax, 16                       
    mov byte ptr gdt_stack32.base_m, al  
    mov byte ptr gdt_stack32.base_h, ah  

    ; получаем адрес сегмента, где лежит глобальная таблица дескрипторов
    mov ax, data32
    shl eax, 4

    add eax, offset gdt_null

    mov dword ptr pdescr+2, eax
    mov word ptr  pdescr, gdt_size-1
    lgdt fword ptr pdescr

    
    mov ax, code32
    mov es, ax

    ; Заносим в дескрипторы прерываний 
    lea eax, es:except_1
    mov idescr_0_12.offs_l, ax
    shr eax, 16
    mov idescr_0_12.offs_h, ax

    lea eax, es:except_13
    mov idescr_13.offs_l, ax 
    shr eax, 16             
    mov idescr_13.offs_h, ax 

    lea eax, es:except_1
    mov idescr_14_31.offs_l, ax 
    shr eax, 16             
    mov idescr_14_31.offs_h, ax 

    
    lea eax, es:new_int08
    mov int08.offs_l, ax
    shr eax, 16
    mov int08.offs_h, ax

    lea eax, es:new_int09
    mov int09.offs_l, ax 
    shr eax, 16             
    mov int09.offs_h, ax 

    ; Получаем линейный адрес IDT
    mov ax, data32
    shl eax, 4
    add eax, offset idt

    ; Записываем в ipdescr линейный адрес IDT (Для з-р) 
    mov  dword ptr ipdescr + 2, eax ; interruption psevdo descriptor
    ; И размер IDT
    mov  word ptr  ipdescr, idt_size-1 
    
    ; Сохранение масок
    in  al, 21h                     
    mov mask_master, al ; Вежущий.       
    in  al, 0A1h                    
    mov mask_slave, al  ; Ведомый.
    
    ; Перепрограммирование ведущего контроллера 
    mov al, 11h
    out 20h, al                     
    mov al, 32
    out 21h, al                     
    mov al, 4
    out 21h, al
    mov al, 1
    out 21h, al

    ; Маска для ведущего контроллера
    mov al, 0FCh ; 1111 1100 - разрешаем только IRQ0 И IRQ1
    out 21h, al

    ; Маска для ведомого контроллера (запрещаем прерывания)
    mov al, 0FFh ;запрещаем все
    out 0A1h, al
    
    ; открытие линии A20 
    in  al, 92h
    or  al, 2
    out 92h, al

    cli ; Запрет аппаратных прерываний. (Маскируемых)
    
    ;  lidt - load IDT - загрузить в регистр IDTR 
    lidt fword ptr ipdescr
    
    ; Запрет немаскируемых прерываний.
    mov al, 80h
    out 70h, al

    ; Переход в защищенный режим
    mov eax, cr0
    or eax, 1   
    mov cr0, eax 

    db  66h  ; Префикс изменения разрядности
    db  0EAh ; far jmp.
    dd  offset pm_start ; Смещение
    dw  code32s         ; Сегмент


return_rm:
    ; возвращаем флаг pe
    mov eax, cr0
    and al, 0FEh                
    mov cr0, eax

    db  0EAh    
    dw  offset go
    dw  code16

go:
    ; обновляем все сегментные регистры
    mov ax, data32   
    mov ds, ax
    mov ax, code32
    mov es, ax
    mov ax, stack32   
    mov ss, ax
    mov ax, stack_size
    mov sp, ax
    
    ; возвращаем базовый вектор контроллера прерываний
    mov al, 11h
    out 20h, al
    mov al, 8
    out 21h, al
    mov al, 4
    out 21h, al
    mov al, 1
    out 21h, al

    ; восстанавливаем маски контроллеров прерываний
    mov al, mask_master
    out 21h, al
    mov al, mask_slave
    out 0A1h, al

    ; восстанавливаем вектор прерываний
    lidt    fword ptr ipdescr16

    ; закрытие линии A20
    in  al, 70h 
    and al, 7Fh
    out 70h, al

    sti ; Резрешаем аппаратные прерывания     
    
    ; Очищаем экран
    call ClearScreen

    mov ah, 09h
    lea dx, pm_msg_out
    int 21h
    call NewLine

    ; Завершаем программу.
    mov ax, 4C00h
    int 21h

    code16_size = $-start  
code16 ends

end start