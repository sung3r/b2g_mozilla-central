/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

package netscape.plugin.composer.io;

/** The token type for html comments. HTML comments, for our purposes, have this form:
 * '&lt;!' + text + '>'.
 */

public class Comment extends Token {
  String text;

  /** Create a comment from a string buffer.
   * @param buf The text of the comment. Doesn't include the '!' at the start of the
   * comment.
   */
  public Comment(StringBuffer buf) {
    text = buf.toString();
  }

  /** Create a comment from a string buffer.
   * @param buf The text of the comment. Doesn't include the '!' at the start of the
   * comment.
   */
  Comment(FooStringBuffer buf) {
    text = buf.toString();
  }

  /** Create a comment from a string.
   * @param buf The text of the comment. Doesn't include the '!' at the start of the
   * comment.
   */
  public Comment(String text) {
    this.text = text;
  }

  /** The text of the comment. Doesn't include the '!' at the start of the
   * comment.
   */
  public String getText() {
    return text;
  }

  /** Return the full html representation of the comment, including
   * the "&lt;!" and the ">".
   */
  public String toString() {
    return "<!" + text + ">";
  }

  /** Compute the hash code for the comment.
   * @return the hash code of the comment.
   */

  public int hashCode() {
    return text.hashCode();
  }

  /** Equality test.
   * @param other the objcet to test for equality with this object.
   * @return true if the text of this comment equals the text of
   * the other comment.
   */
  public boolean equals(Object other) {
    if ((other != null) && (other instanceof Comment)) {
      return text.equals(((Comment)other).text);
    }
    return false;
  }

  /** Create a selection start comment. Equivalent to
   * createSelectionStart(false);
   * @return a canonical selection start comment.
   */

  public static Comment createSelectionStart(){
    return createSelectionStart(false);
  }

  /** Create a selection start comment.
   * @param stickyAfter is true if the selection start sticks to the next token.
   * @return a selection start comment.
   */

  public static Comment createSelectionStart(boolean stickyAfter){
    return new Comment(stickyAfter ? SELECTION_START_PLUS : SELECTION_START);
  }
  /** Create a selection end comment. Equivalent to
   * createSelectionEnd(false);
   * @return a canonical selection end comment.
   */

  public static Comment createSelectionEnd(){
    return createSelectionEnd(false);
  }

  /** Create a selection end comment.
   * @param stickyAfter is true if the selection end sticks to the next token.
   * @return a selection end comment.
   */

  public static Comment createSelectionEnd(boolean stickyAfter){
    return new Comment(stickyAfter ? SELECTION_END_PLUS : SELECTION_END);
  }

  /** Check if this comment is a selection comment.
   * @return true if this comment is a selection comment.
   */
  public boolean isSelection() {
    return isSelectionStart() || isSelectionEnd();
  }

  /** Check if this comment is a selection start comment.
   * @return true if this comment is a selection start comment.
   */
  public boolean isSelectionStart() {
    return text.equals(SELECTION_START) || text.equals(SELECTION_START_PLUS);
  }

  /** Check if this comment is a selection end comment.
   * @return true if this comment is a selection end comment.
   */
  public boolean isSelectionEnd() {
    return text.equals(SELECTION_END) || text.equals(SELECTION_END_PLUS);
  }

  /** Check if this comment is a sticky-after selection comment.
   * @return true if this comment is a selection comment and is a sticky-after comment.
   */
  public boolean isSelectionStickyAfter() {
    return text.equals(SELECTION_START_PLUS) || text.equals(SELECTION_END_PLUS);
  }

  private static final String SELECTION_START = new String("-- selection start --");
  private static final String SELECTION_END = new String("-- selection end --");
  private static final String SELECTION_START_PLUS = new String("-- selection start+ --");
  private static final String SELECTION_END_PLUS = new String("-- selection end+ --");
}
