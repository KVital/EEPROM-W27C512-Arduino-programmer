Це проект програматора W27C512

Прочитати вміт ROM і записати у dump.bin

python programmer.py read COM14 dump.bin

Записати ROM вмістом файлу ROM.bin

python programmer.py write COM14 ROM.bin

Після запису обов'язково перевірити чи все правильно записано

python programmer.py verify COM14 ROM.bin

Якщо потрібно стерти (стирання записує кругом ff). Засис записує лише 0, але не 1

python programmer.py erase COM14

Перевірити стирання (чи усі байти ff)

python programmer.py blank COM14

