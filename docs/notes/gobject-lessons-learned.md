GObject lessons learnt
======================

Just sharing the long journey of learning GObject.



How Do I Learn GObject?
-----------------------

First, you need to bookmark the official documentation. It's the best place to
start, but also a good place to go back afterward. Things that don't make sense
at the first reading will become clear as you go further.

- [GObject Reference Manual](https://developer.gnome.org/gobject/stable/)

Second, there's one essential article that nicely sums up the GObject
construction process, and raise some important points around it. This is the
most important blog post you'll come accross, read it again and again.

- [A gentle introduction to gobject construction](https://blogs.gnome.org/desrt/2012/02/26/a-gentle-introduction-to-gobject-construction/)

At last, you need some good examples. You will find it first within the GObject
library itself, but you will need more. I recommend the main GObject based
libraries, and applications where development is very active. You will find
sometimes that the code is very up-to-date with the latest best practices, so
it's good to look in different codes. Warm up your git, sharpen your grep, and
there you go!

- [GLib](https://gitlab.gnome.org/GNOME/glib)
- [GTK](https://gitlab.gnome.org/GNOME/gtk)
- [GStreamer](https://gitlab.freedesktop.org/gstreamer)
- [Rhythmbox](https://gitlab.gnome.org/GNOME/rhythmbox)
- [Totem](https://gitlab.gnome.org/GNOME/totem)



Good Practices
--------------

#### Follow The Conventions

The first place to learn about good practices is the GObject conventions page.
It's more than good practice actually. GObject expects you to follow a few
conventions, so that code generation tools (`glib-mkenum`) and other stuff can
actually work. Don't try to avoid that.

- [GObject Conventions](https://developer.gnome.org/gobject/stable/gtype-conventions.html)

#### Use `G_DECLARE_*` And `G_DEFINE_*`

Do not clutter the header file with too much boilerplate, as it's often seen in
some old GObject-based code.

- [G_DECLARE_FINAL_DERIVABLE_TYPE](https://blogs.gnome.org/desrt/2015/01/27/g_declare_finalderivable_type/)

#### Do The Minimum In `_init()`

There should be almost nothing in the `init()` function. Most of the time, it
just boils down to setting up the private pointer, if any.

For all the rest, it's better to implement it in the `constructed()` method.

- [A gentle introduction to gobject construction](https://blogs.gnome.org/desrt/2012/02/26/a-gentle-introduction-to-gobject-construction/)

#### Always Use A Private Pointer

You will notice that in here, I never put anything in the object structure. I
ALWAYS use a private structure.

Using a private structure is particularly useful for derivable objects, since
for these objects, the object structure *must be* defined in the header
(public) to allow inheritance. And of course, you want to keep your private
data private, so it makes sense to define it within the implementation,
therefore using a private structure.

But when the object is not derivable, and therefore the object structure
*should be* defined in the implementation, what's the point of having a private
struct?
1. Consistency - you want all your objects to look the same.
2. Code Refactoring - when your objects change from final to derivable, or the
   other way around, you'll be happy that you don't have to constantly move
   your data from the object struct to the priv struct.

- [Changing quickly between a final and derivable GObject class](https://blogs.gnome.org/swilmet/2015/10/10/changing-quickly-between-a-final-and-derivable-gobject-class/)

Also, I *always* use a pointer named `priv` to access it. It's just a
convention, but a very common one, and doing it everywhere makes the whole code
consistent. HOWEVER, I've read somewhere that I shouldn't! But I didn't have
time to dig into that yet.

#### Best Way To Set Properties

The correct way to set an object pointer. It's not as trivial as it seems.

- [Use g_set_object() to simplify (and safetyify) GObject property setters](https://tecnocode.co.uk/2014/12/19/use-g_set_object-to-simplify-and-safetyify-gobject-property-setters/)



Quick FAQ
---------

#### How To Change A Construct-Only Property Via Inheritance

This questions is asked on StackOverflow:

- <http://stackoverflow.com/q/16557905/776208>

Unfortunately, the answer given by Emmanuele Bassi is wrong: setting the
property in the `constructed()` virtual method won't work.

Now that I've been dealing with GObject for a while, I see 3 ways to do that:

- hack around with the `constructor()` method, as suggested in
  <http://stackoverflow.com/a/16592815/776208>.
- don't use construct-only properties, use construct properties. Then you can
  modify it in `constructed()`.
- set the desired values within a wrapper, ie. `my_inherated_object_new()`. If
  you only create your objects with these wrappers, it's fine. However that is
  not binding friendly.

I personally settled for the third solution within Goodvibes.

#### How To Implement A Singleton

This is discussed here and there on the Net.

- [GObject API Reference](https://developer.gnome.org/gobject/unstable/gobject-The-Base-Object-Type.html#GObjectClass)
- [How to make a GObject singleton](https://blogs.gnome.org/xclaesse/2010/02/11/how-to-make-a-gobject-singleton/)

#### How To Have Protected Fields

GObject doesn't provide any solution to have protected fields. I managed to do
without up to now, but just in case, here's a workaround. Basically, it's just
about defining the private structure in the header (therefore making it
public), and naming it 'protected' instead of 'private'.

- [GObject and protected fields – simple hack](http://codica.pl/2008/12/21/gobject-and-protected-fields-simple-hack/)

#### Any Design Patterns Out There?

For design pattern examples, the best is to hunt for it directly in the source
code of some big projects. Additionally, here are some discussions about it.

- <https://mail.gnome.org/archives/gtk-devel-list/2017-August/msg00008.html>
- [GObject design pattern: attached class extension](https://blogs.gnome.org/swilmet/2017/08/09/gobject-design-pattern-attached-class-extension/)



Been there, done that
---------------------

#### Should I Set A Property Asynchronously?

Sometimes, setting a property is just about setting a variable. But at other
times, it might trigger quite a bit of code execution.

For example, at some point I had an `enable` property, and when I set it to
true, it really enables the whole object, and possibly a lot of things happens.

For some technical reasons, I had to actually delay the execution of the
enable/disable code, with `g_idle_add()`. And since I wanted the value of the
`enable` property to really represent the state (enabled or disabled), I
decided to change the value of this property only *after* this asynchronous
code was executed. Ie, only when the `g_idle_add` callback runs. Only then the
`notify` signal was sent.

This seems OK at a first glance. But in practice, there's a problem. The code
that sets the property may not want to receive the notify signal, and therefore
use `g_signal_handler_block()` before setting the property, and
`g_signal_handler_unblock()` after setting it.

If I set the property asynchronously, and therefore send the notify signal
afterwards in a callback, the code that sets the property will receive the
notify signal, even if it tried to block it. So it creates a bit of an
unexpected behavior for the code setting the property.

So... Should I set a property asynchronously? The answer is NO.

For this particular problem of the `enable` property, I solved it by having two
properties:
- `enabled`, readwrite, which reflects how the object is *configured*.
  Therefore this property is set synchronously.
- `state`, readable only, which gives the current state of the object, and go
  from *disabled* to *enabling* to *enabled*. This property is tied to
  `enabled`, but its value changes aynchronously.

With two properties, one can connect to the `enabled` property to know how the
object is configured, and one can connect to the `state` property to know the
real object state.

#### Should I Define A Base Class For My Object Hierarchy?

For example, GStreamer has its own `GstObject`. And at some point, I thought it
was cool, and furthermore it seemed that it would solve some of the problems I
faced. So I defined my own `GvObject` for a while. I then removed it quickly
enough. Here's my feedback from that experience.

First questions that arises: should all my objects inherit from GvObject? Or
only those who have a solid reason for that?

At first, I went for the latter, because I thought that if an object has no
reason to inherit GvObject, then it should remain as lightweight and simple as
possible, and inherit GObject.

So I ended up with a mix of objects that were either GObject derived, either
GvObject derived. It's a bit of a mess suddenly, because the separation between
both, and the reasons for choosing the right parent, was all very unclear.

Then, serialization kicked in, I wanted to be able to serialize objects to save
it as configuration (I didn't use GSettings by the time). And suddenly I had
another dilemna. If the serialization API targets GvObject, then GObject
derived objects were left out, and couldn't be serialized. On the other hand,
if the API targeted GObject, I couldn't take advantage of some of the specific
features GvObject, that were actually very helpful for serializing.

Which made me change my mind: if I define a GvObject base-class, then I should
embrace it completely, and have every object inherit it. It looked nice and
consistent now, but I just added an additional layer, I just made things a bit
more complicated, and I was wondering if it was worth it.

But still, I was happy because now the serialization was easier to implement,
since I embedded some convenient stuff for it within my base class GvObject.

But then, another problem kicked in. The UI part is implemented by inheriting
GTK widgets. How will I serialize this GObject-derived objects, now that the
serialization API targets the GvObject type? My only solution here was to
rewrite the UI, use composition instead of inheritance, so that my UI objects
could be derived from GvObject and be serializable.

Which promised to be quite some work of moving code around...

I didn't do that though. Instead, I came back to GObject for the base class.
For all the problems that I solved with GvObject, I thought again and solved
them differently, and in a better way.

After all that, I draw some conclusions for myself:

- Think twice before defining a base class for your object hierarchy. It has
  its good points, for sure, but also its drawbacks.
- If you work with GObject-based libraries, be aware that the base class you
  define can't help for objects from those libraries, or that you inherit from
  those libraries. Your lower denominator for the whole set of classes you deal
  with will always be GObject. So, does you base type really helps, or does it
  get in the way?
- Ask yourself if you can't solve your problem with an interface instead.
  Interface are more flexible. Even if you use objects from other libraries,
  you can still inherit them, and implement an interface in the child.
