Implementation Decisions - Core
===============================

Some decisions I made early on regarding the implementation. I write it down
here just for my own remembering.



Why Implementing The MPRIS2 D-Bus API First?
--------------------------------------------

Implementing the MPRIS2 D-Bus API was one of the first thing I did. It was not
the easiest thing to start with though, but it forced me to implement all the
common features expected from a music player. Once MPRIS2 was implemented, I
knew I had the right design for the core, and indeed, I never had to rework it
afterward.

So I really recommend that to anyone writing a music player: implement the
MPRIS2 D-Bus API early, it will help you to get the design right, you won't
regret the pain.



Why Not Choosing Python?
------------------------

I definitely wanted to implement the MPRIS2 D-Bus API. Which means implementing
a D-Bus server.

Implementing a fully featured D-Bus server in Python is a bit of a frustrating
experience. Properties are not fully handled. See:

- <https://bugs.freedesktop.org/show_bug.cgi?id=26903>

The quick answer, as I understood it, is that the D-Bus python binding is
broken by design, and there will be no attempt to fix it. This bug will stay
opened forever.

If you have a look at QuodLibet code, you will see that these guys had to write
quite a bit of code to add support for properties. But they did it well, and
the code is easy to take out of QuodLibet. So my guess is that the best way to
fully implement the MPRIS2 D-Bus API is to copy/paste some code from QL.

It's suggested here and there to use another Python D-Bus implementation, like
GLib's implementation GDBus, through PyGObject. This is perfect if you're a
D-Bus client, however if you want to implement a D-Bus daemon? Apparently, at
the time of this writing, this is yet to be implemented. See progress here:

- <https://bugzilla.gnome.org/show_bug.cgi?id=656330>

So, with this situation, I decided to write in C instead, as I was sure that I
would be able to implement the MPRIS2 D-Bus API properly, without having to
hack my way around broken things.

Note that it's probably not an exhaustive view of the situation, and I might
have overlooked other solutions.

Another reason is that Goodvibes is a minimalistic app doing very little, and
it's pleasant to see it start in a fingersnap. With Python you don't quite get
that, especially if you're the first Python app being launched and there's
nothing cached yet.

On the other hand, there was a BIG reason to choose Python rather than C:
development time. Writing in Python is so much faster, you have no idea until
you have the chance to maintain both a C and a Python desktop app.



Why Choosing GObject?
---------------------

GObject has its cons, that I was very aware of when I started to learn it. As
often when I face a new technology, I was a bit reluctant and only saw the
downside:

- Boilerplate: there so much copy/paste to do that I ended up writing a script
  to automate C/H file creation.
- Magic: some of the boilerplate is hidden by macros. It's great since it saves
  code typing, but it's bad since it makes some definitions implicit. So you
  end up calling functions or macros that are defined nowhere, and it's a bit
  unsettling at first. GObject code looks like dark magic for the newcomer.
- Steep learning curve: yep, learning GObject by yourself can be a bit tedious.

On the other hand, GObject brings in everything I needed to implement cleanly
this MPRIS2 D-Bus API:

- signals: from the moment I have signals internally, there's no need to hack
  around to send signals on D-Bus. It's trivial.
- properties: same thing. From the moment the code is GObject-oriented, getting
  and setting properties through D-Bus is trivial.

Plus, GObject allows you to write quite clean object-oriented code in C. The
memory management through reference counting is nice and clean.

And the more you go on working with GObject libraries (GIO, GTK+, GStreamer),
the more you realise that your code benefits from being GObject oriented. You
can use high-level functions, bind your objects here and there, and a lot of
problems are solved by the libraries you use. But these libraries expect
GObjects, so if your code is not written as such, you might be left out and
bitter, rewriting stuff that is already implemented in libraries.

So that's maybe the most important: if your application lives in a GObject
ecosystem, make it GObject, and it will fit in nicely, good surprises will
abound all along.

But be aware that GObject is much more than a library. It really changes the
architecture of your application, which basically becomes event-driven. Which
is exactly what you want when you're writting a GUI application.



Why Not Using Gdbus-Codegen?
----------------------------

First, I'm not a big fan of code generation.

Second, as far as I remember, the generated code produces some warnings at
compile time, and I usually compile with the flag `-WError`. It's annoying.

Third, there's a bug about `Property.EmitsChangedSignal`, that has been pending
forever, and in all likelihood will never be fixed.

- <https://bugzilla.gnome.org/show_bug.cgi?id=674913>

This bug affects the MPRIS2 D-Bus API, so I prefer to stay away from that
`gdbus-codegen` and remain in control of the code.



Why Using Explicit Notify Only?
-------------------------------

This is just an convention in use in the code, I just find it more logical when
I started, and I never had to come back on this choice.

For this reason, every property is created with the `G_PARAM_EXPLICIT_NOTIFY`
flag, and emitted only if the value of the property changed. It's a bit more
job on the object where the properties are changed, but it's less job on the
object listening for signals, since they can be sure that they have to do
something when the signal handler is invoked.



Why Having This Feature Thing?
------------------------------

You will see in the code that some features are quite isolated from the rest of
the code. First, there's a `GvFeature` object, that is the parent of all
features. Then, features just need to inherit this object, connect to some
signals from the core, and that's it.

Isolating features is simply a way to keeps things cleanly separated, and to
allow to disable some code at compile time, without having to cripple the code
with `#ifdef`. It's possible thanks to the fact that features just react to
signals. The core of the code itself is blissfully ignorant of the existence of
these features, and never explictly invokes any function from there.

Features are created once at init, destroyed once at cleanup, and that's the
only place where you will see `#ifdef`. They live in a world of their own.

Such a strong isolation has some benefits: easier debug and maintenance,
mainly. And it's nice to be able to disable it at compile-time, if ever people
want to integrate the application in an environment where some of these
features don't make sense (like on embedded devices).

It also forces a better overall design. However, it can be a pain in the ass
when a new feature shows up, and doesn't quite fit in, and the whole thing
needs to be reworked, and all the existing features need to be updated to match
the changes...

Ultimately, with some improvements, these features could be loaded dynamically,
and from this point they could be called plugins. And then, it's very common to
have plugins in GNU/Linux media player, right? So, these features can be seen
as a poor's man plugin system.



Station List: Shuffle Implementation
------------------------------------

Things I've tried and didn't work.

One idea was that the player would have two StationList objects. One would be
ordered, the other would be shuffled. With that, the 'shuffle problem' is taken
out of the StationList object (which is therefore esier to implement) and is
then solved outside of it. Mainly in the player, I thought.

In practice, it adds quite a lot of complexity, because these two lists must be
kept in sync (from a more genereal point of view, from the moment you duplicate
data, you need to keep it in sync, and that's why duplicating data is always
something to avoid). Furthermore, the station list is a global variable, that
can be accessed by anyone. How to handle duplication then?

We could have both list (ordered and shuffled) global. But it would be a mess
for the rest of the code. Always wondering which one you must deal with, and
what if you want to add a new station? So the code is responsible for adding
it to both list? And therefore, the code is responsible for keeping lists in
sync?

Of course not, the sync between both lists must be automatic, but if both are
global, it means that they both must watch each other's signals to keep in
sync. Or another object must watch both of them and keep them in sync. This
doesn't sound sweet to my ears.

Another possibility is to have only the ordered station list public, and the
shuffled one private to the player, and then the player would be left with the
responsability to keep the shuffled list in sync, somehow. Hmmm, I don't like
that too much either.

OK, so let's forget about that. If the shuffle problem is not to be solved
outside of the list, then let's solve it within the list. We could have a
'shuffle' property added to the station list, and that's all. The code set it
to true or false, and doesn't bother anymore.

It kind of works, but the StationList API becomes confusing. Because if you
look at it as it is, you will notice that some functions are expected to
support the shuffle property (obviously, `next` and `prev` will return a
different value depending on the shuffle property), while other actually don't
care about the shuffle property (`append`, `prepend` only deal with the ordered
list).

So, completely hiding the 'shuffle' from the outside world is also not a very
good solutions. At some point, when you use the API, you start to find it
confusing. Even if you set the shuffle property, some functions keep on dealing
with an ordered list internally. It's magic, and we don't like magic.

Actually, from the user point of view, if you enable shuffle, the list still
appears ordered in the ui. It just affects the previous and next actions. And
the best is to see this behavior appearing in the API.

So in the end, the best implementation of the shuffle I came with is a 50/50:
both the Player object and the StationList object know about it, and both do
their job handling it. It appears explicitely in the StationList API, and only
makes sense for `next` and `prev`. The Player is the 'owner' of the shuffle
setting, and feeds it to StationList when calling methods that require it.

In the end, this implementation is the most simple, and leaves little place for
magic. I've been happy with it so far.
