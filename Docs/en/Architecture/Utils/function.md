## o2::Function<>
For convenient work with subscriptions, the engine has its own delegate type, similar to `std::function<>`.

Unlike the std variant, `o2::Function<>` behaves like C# delegates and is effectively an array of functions.

For example, a delegate for a button click can be declared:
- `o2::Function<void()> onClicked;`

and it can be subscribed/unsubscribed to multiple times:
```C++
button->onClicked += myFunction;
...
button->onClicked += []() { o2Debug.Log("Clicked!"); };
```

When the delegate is invoked (the button is pressed), all subscribers are called.

This delegate is optimized and can be used relatively cheaply. It has an internal optimization similar to small string optimization: if only one function is stored, it is kept inplace without allocations.
