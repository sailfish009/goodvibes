Maintaining
===========



Releasing
---------

Be sure to update everything related to **translation**.

- Use the script `translators.sh` to output the list of translators, then
  update the *About* dialog accordingly.
- Have the `NEWS` reflect the changes.

Then:

- Bump the version in `configure.ac`.
- Git commit, git tag, git push. Done.



Packaging
---------

After releasing, update the Debian packaging files in the goodvibes-debian git
repository. Basically, just bump the changelog, there's nothing else to do.

Git commit, git push. Done

Then, just fire the script `scripts/packaging.sh`, which will fetch everything
from GitHub and build everything that needs to be done. This script is tied to
my config and won't work out of the box on someone else system. But heck, if
you're not me, you're not supposed to release anything anyway, so move on !

At last, a few `dput` commands will finish the damn job. Done with packaging.



Translations
------------

Before every release, it's time to update everything regarding translations, so
that translators have a chance to do their homework !

First, ensure that the file `po/POTFILES.in` is still up to date. This is still
a manual process, but it wouldn't be that hard to write a little script.

Then, and before modifying the po files on our side, synchronize with Weblate
and lock it. We can do it either via the web interface, either via the client.

	# Lock Weblate translation
	wlc lock
	# Push changes from Weblate to upstream repository
	wlc push
	# Pull changes from upstream repository to your local copy
	git pull

Then, update the *translation template*, aka. `po/goodvibes.pot`, along with
the *message catalogs*, aka. the po files.

	# Update translation files
	make -C po update-po
	git add po/*.{po,pot}
	git commit -m"i18n: update goodvibes.pot and po files"
	# Push changes to upstream repository
	git push

At last, we can unlock Weblate and update it.

	# Tell Weblate to pull changes (not needed if Weblate follows your repo automatically)
	wlc pull
	# Unlock translations
	wlc unlock

For more details, refer to the weblate workflow as described at:
<https://docs.weblate.org/en/latest/admin/continuous.html>.



Artwork
-------

Both svg source files and png files are versioned. To update the png files, run
`make icons`.
