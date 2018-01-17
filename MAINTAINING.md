Maintaining
===========



Releasing
---------

Be sure to update everything related to **translation**.

- Use the script `translators.sh` to output the list of translators, then
  update the *About* dialog accordingly.
- Have the `NEWS` reflect the changes.
- TODO: document how to update the po file.



Translations
------------

Before every release, it's time to update everything regarding translations, so
that translators have a change to do their homework !

First, ensure that the file `po/POTFILES.in` is still up to date. This is still
a manual process, but it wouldn't be that hard to write a little script.

Then, update the *translation template*, aka. `po/goodvibes.pot`.

	make -C po update-po
	git add po/goodvibes.pot
	git commit -m"i18n: update goodvibes.pot"

	# We didn't want to modify the po files, they're managed by Weblate.
	git checkout po/*.po

Done, just push.

	git push

The weblate workflow is described at <https://docs.weblate.org/en/latest/admin/continuous.html>.



Artwork
-------

Both svg source files and png files are versioned. To update the png files, run
`make icons`.
