MAINTAINING
===========



Updating translations
---------------------

Before every release, it's time to update everything regarding translations, so
that translators have a chance to do their homework !

First, ensure that the file `po/POTFILES` is still up to date. This is still a
manual process, but it wouldn't be that hard to write a little script.

Then, and before modifying the po files on our side, synchronize with Weblate
and lock it. We can do it either via the client `wlc`, either via the web
interface at <https://hosted.weblate.org/projects/goodvibes/translations/>,
under *Manage -> Repository maintenance*.

    # Lock Weblate translation
    wlc lock
    # Make Weblate push its changes to the git repo
    wlc push
    # Pull changes from the git repo to your local copy
    git pull

Then, update the *translation template*, aka. `po/goodvibes.pot`, along with
the *message catalogs*, aka. the po files.

    # Update the pot file and the po files
    [ -d build ] || meson build
    ninja -C build goodvibes-pot
    ninja -C build goodvibes-update-po

    # Commit changes
    git add po/*.{po,pot}
    git commit -m"i18n: Update goodvibes.pot and po files"

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

    ./scripts/print-translators.sh code
    vi src/ui/gv-about-dialog.c
    git commit -am"ui: Update translation credits"

    ./scripts/print-translators.sh doc
    vi docs/goodvibes.readthedocs.io/credits.rst
    git commit -am"doc: Update translation credits"

    vi NEWS
    git commit -am"news: Update translations"

#### Release

- Ensure `NEWS` is up-to-date (check Git history and GitLab milestones).
- Add a new release to the appdata file.
- Bump the version in `meson.build`.
- Git commit, git tag, git push.

In bash, here you go:

    # Definitions
    VER=0.4.3
    TAG=v${VER:?}
    DATE=$(date -u +%Y-%m-%d)

    # Edit the news
    vi NEWS

    # Add a release to appdata
    sed -i "/<releases>$/a\ \ \ \ <release version=\"${VER:?}\" date=\"${DATE:?}\"/>" \
        data/*.appdata.xml.in

    # Bump version in meson.build
    sed -i -E "s/ version: '(.*)'/ version: '${VER:?}'/" meson.build

    # Commit and push
    git add NEWS data meson.build
    git commit -m "Release version ${VER:?}"
    git tag "${TAG:?}"
    git push && git push --tags

#### Move on

Edit the file `meson.build` and bump the version to *something else* to avoid
confusion. These days I just append `+dev`.

Head to the *Milestones* page on GitLab, available at
<https://gitlab.com/goodvibes/goodvibes/-/milestones>, close the current
milestone, and maybe create a new one?



Packaging
---------

#### Debian

Let's move to the `goodvibes-debian` git repository from now on, available at
<https://gitlab.com/goodvibes/goodvibes-debian>

Make sure that the `DEB*` variables are all set.

    export DEBFULLNAME=$(git config user.name)
    export DEBEMAIL=$(git config user.email)

For a new release, the only thing needed is just to bump the changelog.

    dch --newversion ${VER:?}-0goodvibes1 "New upstream release."
    dch --release

Git commit, git push. Done.

    git add debian/changelog
    git commit -m "Version ${VER:?}"
    git push

Then, just fire the script `debian/build-all.sh`, and sign the resulting files.

    ./debian/build-all.sh release

The script uses `sbuild` to build the Debian binary packages. It should work
for you if you have the sbuild chroots ready for the suites that the script
targets (at the moment, `buster` and `sid`).

At last, a few `dput` commands will finish the damn job. Done with packaging.

#### Flathub

Let's move to the `io.gitlab.Goodvibes` git repository from now on, available at
<https://github.com/flathub/io.gitlab.Goodvibes.git>.

The only thing to do is to checkout a test branch, bump the version, set the
commit, and maybe update the runtime version. Then push the branch.

    git checkout -b test-${VER:?}
    vi io.gitlab.Goodvibes.yaml    # set tag and commit
    git commit -am "New upstream release ${VER:?}"
    vi io.gitlab.Goodvibes.yaml    # bump runtime version
    git commit -am "Bump runtime version to ..."
    git push -u origin test-${VER:?}

Now, let's create a pull request with the words `bot, build`. This will
trigger a test build on Flathub. Watch your mailbox for notifications, it's
very fast.

If the build is successful, the app will be available for 5 days in the test
repo, which gives enough time to install it and test it. If it works, just
merge the PR. Flathub will notice the activity on `master` and trigger a build.
It can take a few hours, and there's no notification this time, so just refresh
<https://flathub.org/apps/details/io.gitlab.Goodvibes> until it's done.



Artwork
-------

Both the SVG source files and the PNG files are versioned. To rebuild the PNG
files, run:

    ./scripts/build-png-images.sh all



Documentation
-------------

We use the [font-logos](https://github.com/lukas-w/font-logos) to display logos
of various Linux distros in the RTD documentation. This font is maintained and
has new releases on a regular basis. So we probably want to update the copy of
the font that we keep at `goodvibes.gitlab.io/fonts`.



Integrations
------------

This is just for myself to remember what additional services are used for
Goodvibes development, and how it integrates with GitLab.

#### GitLab CI

Up to date documentation should be available on the *Registry* page:
<https://gitlab.com/goodvibes/goodvibes/container_registry>

The configuration is mostly in-tree:
- `.gitlab-ci.yml` describes the different pipelines.
- `.gitlab-ci` contains the Dockerfiles.

Images can be rebuilt using `scripts/build-docker-image.sh`, and run using
`scripts/run-docker-image.sh`. See the usage message for details.

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
