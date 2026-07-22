## Задачи
`o2::TaskManager` — синглтон для запуска отложенных и покадрово обновляемых задач. Глобальная точка доступа — макрос `o2Tasks`. Менеджер обновляет все задачи каждый кадр и удаляет завершенные.

## Методы
- **Run(update, isDone)** — запускает функциональную задачу: `update(dt)` вызывается каждый кадр, пока `isDone()` не вернет `true`.
- **Run(update, time)** — запускает задачу на заданное время в секундах.
- **Invoke(func, delay)** — вызывает функцию один раз после задержки.
- **StopTask(id)** / **StopAllTasks()** — останавливает задачу по id или все задачи.
- **FindTask(id)** — возвращает задачу по id.

## Классы задач
Базовый класс — `o2::Task` (регистрируется в менеджере, имеет `Update(dt)`, `IsDone()`, `ID()`). Наследники: `FunctionalTask`, `TimeTask`, `FunctionalTimeTask`, `DelayedTask`, `FunctionalDelayedTask`. Можно наследовать собственные задачи от `Task`.

Колбэки задаются делегатами [`o2::Function<>`](/Docs/ru/Architecture/Utils/function.md).

<details>
<summary>Пример</summary>

```C++
o2Tasks.Invoke([]() { o2Debug.Log("After 1 second"); }, 1.0f);

o2Tasks.Run([](float dt) { /* каждый кадр в течение 2 секунд */ }, 2.0f);

o2Tasks.Run([](float dt) { /* обновление */ },
            []() { return someCondition; }); // до выполнения условия
```
</details>
