# Contributing to the Improved AtTiny85 Tetris Game

We welcome contributions to the Improved AtTiny85 Tetris Game project. By contributing, you can help improve the game, fix bugs, add new features, or optimize existing code.

## How to Contribute

1. Fork the repository to your own GitHub account.
2. Clone the forked repository to your local machine.
3. Create a new branch for your changes.
4. Make your changes and ensure that the code is well-documented with comments.
5. Test your changes thoroughly to ensure they work as expected.
6. Commit your changes with a clear and concise commit message.
7. Push your changes to your forked repository.
8. Submit a pull request to the main repository.

## Modular and Clean Approach

We encourage a modular and clean approach to code contributions. This means that each function or module should have a single responsibility and be well-documented with comments. Comments should be placed after the closing bracket of the function or module, explaining its purpose and behavior.

Here's an example of how to document a function:

```c
// This function checks if the current piece can move to the specified position
// without colliding with existing pieces or the game boundaries.
//
// Parameters:
//   dx: The horizontal offset to check (negative for left, positive for right)
//   dy: The vertical offset to check (negative for up, positive for down)
//
// Returns:
//   true if the piece can move to the specified position, false otherwise
bool canMove(int8_t dx, int8_t dy) {
    // Function implementation here
}
```

## Code Style

Please follow the existing code style and formatting conventions in the project. This includes indentation, spacing, and naming conventions.

## Testing

Before submitting a pull request, please ensure that your changes have been thoroughly tested. This includes testing on different hardware configurations and verifying that the game behaves as expected in all scenarios.

## License

By contributing to this project, you agree that your contributions will be licensed under the MIT License.