MAINTAINING
===========



Updating translations
---------------------

Before every release, it's time to update everything regarding translations, so
that translators have a chance to do their homework !

First, ensure that the file `po/POTFILES` is still up to date. This is still a
manual process, but it wouldn't be that hard to write a little script.

Then, and before modifying the po files on our side, synchronize with Weblate
and lock it. We can do it either via the web interface, either via the client.

    # Lock Weblate translation
    wlc lock
    # Make Weblate push its changes to the git repo
    wlc push
    # Pull changes from the git repo to your local copy
    git pull

Then, update the *translation template*, aka. `po/goodvibes.pot`, along with
the *message catalogs*, aka. the po files.

    # Update the pot file and the po files
    cd build
    ninja goodvibes-pot
    ninja goodvibes-update-po

    # Commit changes
    cd ..
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



Releasing
---------

#### Translation bits

Be sure to update everything related to **translation** (see above).

Only then you can go on with:

- Update the translator list in the *About* dialog.
- Update the translator list in the documentation.
- Write down changes in the `NEWS` file.

In bash, it translates to something like that:

    ./scripts/translators.sh code
    vi src/ui/gv-about-dialog.c
    git commit -am"ui: update translation credits"
    ./scripts/translators.sh doc
    vi docs/goodvibes.readthedocs.io/credits.rst
    git commit -am"doc: update translation credits"
    vi NEWS
    git commit -am"Update translations in changelog"

Then:

- Ensure `NEWS` is up-to-date (check Git history and GitLab milestones).
- Bump the version in `meson.build`.
- Git commit, git tag, git push.

In bash, here you go:

    vi NEWS
    v=0.3.5
    sed -i -E "s/version: '([0-9.]*)'/version: '$v'/" meson.build
    git add NEWS meson.build
    git commit -m "Bump version to ${v:?}"
    git tag "v${v:?}"
    git push && git push --tags

Done.



Packaging
---------

After releasing, update the Debian packaging files in the `goodvibes-debian`
repository. Basically, just bump the changelog, there's nothing else to do.

    DEBFULLNAME=$(git config user.name) \
    DEBEMAIL=$(git config user.email) \
    dch --distribution $(dpkg-parsechangelog --show-field Distribution) \
        --newversion ${v:?}-0ebo1

Git commit, git push. Done

    git add debian/changelog
    git commit -m "Version ${v:?}"
    git push

Then, just fire the script `debian/build-all.sh`.

    export DEBFULLNAME=$(git config user.name)
    export DEBEMAIL=$(git config user.email)
    ./debian/build-all.sh

This script is tied to my config and won't work out of the box on someone else
system. But heck, if you're not me, you're not supposed to release anything
anyway, so move on!

At last, a few `dput` commands will finish the damn job. Done with packaging.



Artwork
-------

Both the svg source files and the png files are versioned. To rebuild the png
files, run:

    ./scripts/rebuild-images.sh icons



Integrations
------------

This is just for myself to remember what additional services are used for
Goodvibes development, and how it integrates with GitLab.

#### GitLab CI

Up to date documentation should be available on the *Registry* page:
<https://gitlab.com/goodvibes/goodvibes/container_registry>

The configuration is mostly in-tree:
- `.gitlab-ci.yml` describes the different pipelines
- `.gitlab-ci` contains Dockerfiles and associated scripts.

The docker images need to be updated from time to time. The procedure to update
an image is roughly:

    DIST=debian
    cd .gitlab-ci

    # Work on the image
    vi Dockerfile.${DIST:?}
    # Build the image
    ./docker-build-image.sh Dockerfile.${DIST:?}
    # Log in the registry
    docker login registry.gitlab.com
    # Push the image
    docker push registry.gitlab.com/goodvibes/goodvibes/${DIST:?}

#### GitHub Mirror

Very well documented at: <https://docs.gitlab.com/ee/workflow/repository_mirroring.html>.
In short:

1. Create a GitHub token.
2. Configure GitLab to push automatically, using the token to authenticate.

This mirror is just there for a while, so that active GitHub users (if any)
have time to notice the change.

#### WebLate

Weblate needs to be notified of new commits, so there's a webhook.

Additionally, Weblate needs write permission on the repository. This is
achieved by adding the [Weblate push user](https://gitlab.com/weblate) as a
member of the project. I configured it as a `Developer`, however developers
can't write to protected branches by default, so there's a bit of additional
config.

#### ReadTheDocs

Just needs to be notified of changes, so there's only a webhook.
