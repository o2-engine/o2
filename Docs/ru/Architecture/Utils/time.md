## Время
`o2::Time` — синглтон системы времени, обновляется приложением каждый кадр. Глобальная точка доступа — макрос `o2Time`.

## Методы
- **GetApplicationTime()** — время работы приложения в секундах.
- **GetLocalTime()** / **SetLocalTime(time)** / **ResetLocalTime()** — локальное время, управляемое пользователем.
- **GetCurrentFrame()** — индекс текущего кадра.
- **GetDeltaTime()** — дельта времени кадра.
- **GetFPS()** — усредненный FPS.
- **CurrentTime()** — текущее системное время как `TimeStamp`.

## Timer
`o2::Timer` — таймер для замера интервалов, имеет платформенные реализации:
- **Reset()** — сброс таймера.
- **GetTime()** — время в секундах с последнего `Reset()`.
- **GetDeltaTime()** — время с последнего `Reset()` или `GetDeltaTime()`.

`o2::ScopeTimer` — вспомогательный таймер, печатающий время жизни своего скоупа в консоль.

## TimeStamp
`o2::TimeStamp` — сериализуемая структура с датой и временем (год, месяц, день, час, минута, секунда).

<details>
<summary>Пример</summary>

```C++
float dt = o2Time.GetDeltaTime();
o2Debug.Log("FPS: %f", o2Time.GetFPS());

Timer timer;
DoSomethingLong();
o2Debug.Log("Elapsed: %f sec", timer.GetTime());
```
</details>
