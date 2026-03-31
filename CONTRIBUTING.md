# Contributing

## PR naming convention

This project follows the
[Conventional Commit specification v1.0.0](https://www.conventionalcommits.org/en/v1.0.0/).
You can use the following commit types:

- All types recognized by the
  [Angular convention](https://github.com/angular/angular/blob/22b96b9/CONTRIBUTING.md#type)
- `content:` for PRs that only introduce Markdown content
- `revert:` for reverted changes

You should also try to specify one of the following scopes if applicable:

- `local` for changes made to the local offline part
- `serve` for changes made to the server shell part
- `nix` for changes related to Nix packaging.
