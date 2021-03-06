# Spreadsheet
## Общее описание
 
### Ячейки

Таблица состоит из ячеек. Для пользователя ячейки задаются своим индексом, т.е. строкой вида "А1" или "С14", или "RD2". Программно положение ячейки описывается позицией, т.е. номерами её строки и столбца, которые представлены классом Position. Причём номера строк и столбцов начинаются с нуля, как это принято в С++. Например, индексу "А1" соответствует позиция (0, 0), а индексу "AB15" - позиция (14, 27). Преобразования между индексом и позицией осуществляются методами Position::FromString() и Position::ToString().

Для определённости мы будем полагать, что количество строк и столбцов в нашей таблице не будет превышать 16384. То есть максимальная позиция ячейки составит (16383, 16383) с индексом "XFD16384".

Для получения ячейки с заданной позицией используется метод ISheet::GetCell(). Он возвращает указатель на объект класса ICell. С его помощью можно получить текст и значение ячейки. Текст - это "сырое" содержимое ячейки, а значение - это "отображаемое" содержимое. В существующих решениях текст доступен только во время редактирования ячеек, а значения отображаются всё остальное время.

Ячейка считается пустой, если вызов GetCell() вернул nullptr, либо если её текст пуст.
В существующих решениях ячейка может содержать данные множества разных форматов: простые текстовые данные, численные значения, валюта, дата, и т.п. В данной задаче я ограничился только двумя типами: простой текст (std::string) и числа с плавающей запятой (double). Причём последние могут появиться только как результат вычисления значения формул.

### Формулы

Ячейка трактуется как формула в случае, если её текст начинается со знака "=" (и на этом не заканчивается, то есть текст, которые содержит только этот знак, формулой не считается). То, что следует после знака "=", называется выражением формулы. Выражение формулы - это простое арифметическое выражение, которое может содержать следующие элементы:

•	Числа (5, 3.14)

•	Бинарные операции (1*3+4/2-5)

•	Ссылки на другие ячейки (А1+B2*C3)

В отличие от существующих решений, результатом вычисления формулы может быть только число. То есть формула не может быть использована, например, для конкатенации строк. Если формула ссылается на другую ячейку, которая не содержит формулу, то её текст трактуется как число. Если эта ячейка пустая, то её значение полагается равным нулю.
Получается, что, если в ячейке записана формула, то текстом ячейки является знак равенства и выражение формулы, а значением ячейки является число - вычисленное значение формулы. Если в ячейке записан простой текст, то значение ячейки обычно совпадает с её текстом. Кроме случая, когда текст начинается с символа "'" (апостроф). Тогда в значении ячейки этот символ не присутствует. Это можно использовать, если нужно начать текст со знака "=", но чтобы он не интерпретировался как формула.
Содержимое ячейки задаётся методом ISheet::SetCell(), в который передаётся текст ячейки. Далее реализация сама решает, как интерпретировать этот текст.

### Вставка и удаление строк и столбцов

Важно отметить, что в таблицу можно вставлять строки и столбцы и удалять их. Поддержка такой операции делает реализацию значительно более интересной. Рассмотрим такой пример (здесь приведены тексты ячеек):
| | A | B |
|----|:----:|:----:|
|1| 42| 43|
|2|=A1|=A2+B1|

Если вызвать для такой таблицы InsertCols(1) (то есть вставить столбец перед столбцом "B", который является первым столбцом, если считать с нуля), то мы получим таблицу 
| | A | B | C |
|:----:|:----:|:----:|:----:|
|1| 42| | 43|
|2|=A1| |=A2+B1|

Обратите внимание, что ячейка "B2" теперь "переехала" в "С2", и её текст изменился. Имеет смысл пользоваться ментальной моделью, в соответствии с которой "B1" до вставки и "С1" после вставки - суть одна и та же ячейка, у которой просто изменился индекс. И подобные изменения индекса должны быть отражены во всех релевантных формулах.
Таким образом, при вставке строк или столбцов могут изменяться тексты ячеек с формулами, но значения ячеек не меняются.

С удалением ячеек дело обстоит чуть сложнее. Рассмотрим тот же пример, только вызовем для него DeleteCols(0) (то есть удалить столбец "A"). Тогда мы получим следующую таблицу:
| | A |
|:----:|:----:|
|1| 42|
|2|=#REF!+A1|

Второе слагаемое в бывшей ячейке "B2" обновилось корректно, а первое заменилось на "#REF!". Это сообщение об ошибке, которое значит, что мы пытаемся обратиться к недоступной ячейке. Что логично, она ведь была удалена. Вычисление значения такой формулы вернёт нам не численное значение, а эту самую ошибку. Это отражено в типе ICell::Value - он может содержать либо простой текст, либо число, либо ошибку.

Таким образом, при удалении строк или столбцов могут изменяться как тексты ячеек с формулами, так и их значения. При этом, в качестве новых значений могут фигурировать сообщениями об ошибках.

### Печать таблицы

С помощью методов ISheet::PrintValues() и ISheet::PrintTexts() таблицу можно распечатать целиком в некоторый поток. При этом будет выведена минимальная прямоугольная область, включающая в себя все непустые ячейки. Размер этой области можно запросить в явном виде с помощью метода ISheet::GetPrintableSize().

Для печати формул предназначен метод ISheet::GetExpression(), который возвращает текстовое представление формулы без пробелов и лишних скобок.

### Обработка ошибок

Электронная таблица - достаточно гибкая структура, и её легко можно привести в неконсистентное состояние. Реализация должна корректно вести себя в случае всех возможных ошибок.
Могут быть выброшены следующие исключения:

•	Некорректная позиция. Программно есть возможность создать экземпляр класса Position, в котором хранится некорректная позиция. Например (-1, -1). Попытка передать такую позицию в методы предоставляемых интерфейсов должна приводить к исключению InvalidPositionException. Гарантируется, что позиции, возвращаемые методами интерфейсов (например, ICell::GetReferencedCells()) всегда корректны.

•	Некорректная формула. Если в ячейку с помощью метода ISheet::SetCell() пытаются записать синтаксически некорректную формулу (например, =A1+*), то реализация должна выбросить исключение FormulaException, а значение ячейки не должно измениться. Формула считается синтаксически некорректной, если она не удовлетворяет предоставленной грамматике.

•	Циклическая зависимость между ячейками. Если в ячейку с помощью метода ISheet::SetCell() пытаются записать формулу которая привела бы к циклической зависимости между ячейками, то реализация должна выбросить исключение CircularDependencyException, а значение ячейки не должно измениться.

•	Слишком большая таблица. Напрямую нельзя создать ячейку с позицией превышающей максимальную, равно как и указать такую ячейку в формуле. Однако можно заполнить ячейку близкую к максимальной, или сослаться на таковую в формуле, а затем вставить в таблицу ещё несколько строк/столбцов, тем самым "выдвинув" ячейку за пределы максимальной позиции. Программа не должна позволять так делать, а вместо этого должна бросать исключение TableTooBigException.
Значение ячейки с формулой может принимать следующие ошибочные состояния, описанные в классе FormulaError:

•	#REF! - Ссылка на несуществующую ячейку. См. пример выше про удаление ячеек.

•	#VALUE! - Ссылка на ячейку, которая не является формулой и не может быть трактована как число; необходимо, чтобы вся ячейка трактовалась как число, то есть ячейка с текстом "11PM" приведёт к данной ошибке.

•	#DIV/0! - В процессе вычисления значения формулы возникло деление на ноль, или переполнился тип double.
При этом, если формула в ячейке зависит от формулы в другой ячейке, вычисление которой привело к ошибке, то текущая формула вернёт ту же ошибку. То есть ошибки "распространяются" наверх по зависимостям. Если формула в ячейке зависит от формул в нескольких ячейках, и они возвращают разные ошибки, то текущая формула может вернуть любую из этих ошибок.

### Быстродействие и сложность

Сложность работы реализации. Для удобства её оценки, давайте сделаем несколько предположений:

•	Количество ячеек, на которые ссылается произвольная формула, не превосходит некоторой константы

•	Общее количество ссылок на ячейки вне печатной области ограничено некоторой константой

Далее давайте скажем, что K - количество ячеек в печатаемой области таблицы. Тогда от реализации мы ожидаем следующих асимптотических сложностей работы в разных сценариях:
|#| Сценарий | Сложность |
|:----:|:----|:----|
|1|Вызов любого метода интерфейса ISheet|O(K)|
|2|Вызов метода ISheet::GetCell()|O(1)|
|3|Вызов метода ICell::GetValue()|O(K)|
|4|Вызов метода ICell::GetText()|O(1)|
|5|Вызов метода ICell::GetReferencedCells()|O(1)|
|6|Повторный вызов метода ISheet::SetCell() с теми же аргументами|O(1)|
|7|Повторный вызов метода ICell::GetValue() при условии, что значения ячеек, от которых данная ячейка зависит напрямую или опосредованно, не менялись с момента первого вызова (вставка строк и столбцов не приводит к изменению значений ячеек)|O(1)|

### Кеширование вычислений

Давайте рассмотрим интересный пример, где с помощью формул мы вычисляем в таблице значения треугольника Паскаля:
| | A | B | C | D |
|:----:|:----:|:----:|:----:|:----:|
|1| 1| | | |
|2|=A1|=Aa+B1| | |
|3|=A2|=A2+B2|=B2+C2	| |
|4|=A3|=A3+B3|=B3+C3|=C3+D3|
|5|...|...|...|...|...|

Каждая (ну, почти каждая) ячейка зависит от двух ячеек в предыдущей строке. Это значит, что каждая ячейка опосредованно зависит от O(K) других ячеек. Теперь пусть мы вызываем метод ISheet::PrintValues(), который вычисляет значения всех ячеек. При наивной реализации без кеширования вычисление одной ячейки будет иметь сложность O(K). Таким образом, общая сложность работы метода составит O(K^2), что противоречит требованию #1.

Кроме того, из требования #7 напрямую следует необходимость сохранять кеш значений ячеек.

Если значение ячейки изменилось, то кеш значений всех ячеек, что от неё зависят непосредственно или опосредованно, должен быть инвалидирован. Причём требование #6 говорит нам, что это должно происходить быстро.

### Парсинг формул

Я использовал готовую грамматику и все нужные для работы ANTLR файлы, мне только пришлось реализовать бизнес-логику формул. Для этого я реализовал listener, который строит AST формулы из моих собственных классов, и дальнейшие манипуляции уже проводил с этим деревом. 

### Граф зависимостей

Для эффективной инвалидации кешированных значений можно использовать граф зависимостей. То есть для каждой ячейки нам нужно знать, от каких ячеек зависит она (исходящие ссылки), и какие ячейки зависят от неё (входящие ссылки).
При изменении значения ячейки достаточно пройтись рекурсивно по всем входящим ссылкам и инвалидировать кеши соответствующих ячеек. Причём, если кеш какой-то ячейки уже был инвалидирован, нет смысла продолжать рекурсию дальше. Именно эта оптимизация и позволит достичь константной сложности в требовании #6. Кроме того, граф зависимостей упростит предотвращение циклических зависимостей.
Вершинами данного графа являются ячейки, и хранить его удобно как список рёбер, входящих в и исходящих из конкретной ячейки. При изменении ячейки необходимо будет обновлять список исходящих ссылок (рёбер), а также списки входящих ссылок (рёбер) для всех ячеек, от которых данная ячейка зависела и станет зависеть.

