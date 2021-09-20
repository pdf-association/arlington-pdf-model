/*
 * XMLQuery.java
 * Copyright 2020 PDF Association, Inc. https://www.pdfa.org
 *
 * This material is based upon work supported by the Defense Advanced
 * Research Projects Agency (DARPA) under Contract No. HR001119C0079.
 * Any opinions, findings and conclusions or recommendations expressed
 * in this material are those of the author(s) and do not necessarily
 * reflect the views of the Defense Advanced Research Projects Agency
 * (DARPA). Approved for public release.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Contributors: Roman Toda, Frantisek Forgac, Normex. Peter Wyatt, PDF Association
 */
package gcxml;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import static java.lang.Integer.max;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 *
 * @author fero
 */
public class TSVUpdater {
    /**
    * The list of valid supported PDF versions
    */
    final double[] pdf_version = {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 2.0};
    
    /**
    * The path to the latest TSV file set (typically "tsv/latest")
    */
    static String path_to_tsv_files = "";
    
    /**
    * Constructor. Deletes existing PDF version TSV sub-folders!
    */
    public TSVUpdater(){
        path_to_tsv_files = System.getProperty("user.dir") + "/tsv/latest/";
        for(double x : pdf_version){
            deleteContent(x);
            createTSV(x);
        }
    }
    
    /**
    * Creates a TSV file set for the specified PDF version
    * @param version  the PDF version between 1.0 to 2.0 inclusive
    */
    private static void createTSV(double version) {
        final String delimiter = "\t";
        
        File folder = new File(path_to_tsv_files);
        var list_of_files = folder.listFiles();
        
        for (File file : list_of_files){
            if (file.isFile() && file.canRead() && file.exists()) {
                try {
                    BufferedReader tsv_reader;
                    var temp = path_to_tsv_files;
                    String file_name = file.getName().substring(0, file.getName().length()-4);
                    temp += file_name + ".tsv";
                    tsv_reader = new BufferedReader(new FileReader(temp));
                    String[] row;
                    String output_string = "";
                    String entry = "";
                    System.out.println("================\nProcessing " + file_name + " for version " + version);
                    String current_line = tsv_reader.readLine();
                    // First line is header
                    if (current_line != null) {
                        output_string = current_line + "\n";
                    }
                    while ((current_line = tsv_reader.readLine()) != null) {
                        row = current_line.split(delimiter, -1);
                        if (row.length != 12) {
                            System.out.println("Error: " + file_name + " had " + row.length + " rows, not 12!\n" + row);
                        } 
                        else {
                            String key_name = row[0];
                            // Type: SEMI-COLON separated, may have version-based predicates
                            String data_type = row[1]; 
                            // SinceVersion: 1.0, 1.1, ..., 2.0 inclusive 
                            String since_version = row[2];
                            String deprecated = row[3];
                            // Required: possibly wrapped in "fn:IsRequired(...)" with version-based predicates
                            String required = row[4];
                            String indirect_ref = row[5];
                            String inheritable = row[6];
                            String default_value = row[7];
                            // PossibleValues: complex form [];[];[], may have version-based predicates
                            String possible_values = row[8];
                            String special_case = row[9];
                            // Links: complex form [];[];[], may have version-based predicates
                            String links = row[10];
                            String notes = row[11];
                            
                            if (Double.parseDouble(since_version) <= version) {
                                System.out.println("\tKept key: " + key_name);
                                String types_reduced = reduceTypesForVersion(data_type, version);
                                String links_reduced = reduceComplexForVersion(links, version);
                                String pv_reduced = reduceComplexForVersion(possible_values, version);
                               
                                if (required.startsWith("fn:IsRequired(")) {
                                    required = ReduceRequiredForVersion(required, version);                                    
                                }

                                String record =
                                        key_name + delimiter +
                                        types_reduced + delimiter +
                                        since_version + delimiter +
                                        deprecated + delimiter +
                                        required + delimiter +
                                        indirect_ref + delimiter +
                                        inheritable + delimiter +
                                        default_value + delimiter +
                                        pv_reduced + delimiter +
                                        special_case + delimiter +
                                        links_reduced + delimiter +
                                        notes;
                                entry += record + "\n";
                            }
                            else {
                                System.out.println("\tDropped key: " + key_name);
                            }
                        }            
                    } // while
                    // Did we exclude the entire object??
                    if (!entry.isEmpty()) {
                        output_string += entry;
                        WriteToFile(output_string, file_name, version);
                    }
                    else {
                        System.out.println("\tNot writing file " + file_name + " for version " + version);                        
                    }
                } 
                catch (FileNotFoundException ex) {
                    Logger.getLogger(TSVUpdater.class.getName()).log(Level.SEVERE, null, ex);
                } 
                catch (IOException ex) {
                    Logger.getLogger(TSVUpdater.class.getName()).log(Level.SEVERE, null, ex);
                } // catch
            } // if file OK
        } // for file
    }

    /**
    * Writes a new TSV file to a specified PDF version folder
    * 
    * @param output_string string the tab-separated multi-line string to write
    * @param file the filename (i.e. PDF object)
    * @param version the PDF version used to select the sub-folder
    */
    private static void WriteToFile(String output_string, String file, double version) {
        BufferedWriter out = null;
        
        String path = System.getProperty("user.dir") + "/tsv/";
        path += version + "/" + file +".tsv";

        try {
            FileWriter fstream = new FileWriter(path, false);
            out = new BufferedWriter(fstream);
            out.write(output_string);
        }
        catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
        }
        finally {
            if (out != null) {
                try {
                    out.close();
                } catch (IOException ex) {
                    Logger.getLogger(TSVUpdater.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }

    /**
    * Deletes an entire PDF version specific folder of all Arlington TSV files
    * 
    * @param x  the PDF version (1.0, 1.1, ..., 2.0)
    */
    private void deleteContent(double x) {        
        String path = "";
        path +=  System.getProperty("user.dir") + "/tsv/";
        path += x + "/";
        
        File folder = new File(path);
        var list_of_files = folder.listFiles();
        
        for (File file : list_of_files) {
            file.delete();
        }
    }
    
    /**
     * Finds the matching closing bracket ")" for the first open bracket "(" 
     * in a string. 
     * e.g. (abc) = 4
     *      fn((a==b),(c && (d!=e))) = 24
     * 
     * @param s  the string
     * @return -1 if no matching bracket pair or IndexOf matching ")" in string
     */
    private static int IndexOfOuterCloseBracket(String s) {
        int nested = 0;
        int i = 0;
        
        for (char ch: s.toCharArray()) {
            if (ch == '('){
                nested++;
            }
            else if (ch == ')') {
                if (nested == 1) {
                    return i;
                }
                nested--;
            }
            i++;
        }
        return -1;
    }

    
    /**
    * Processes an atomic Arlington entry that might contain version
    * predicates and reduces it appropriately for the specified PDF version.
    *
    * Version-specific predicates ONLY that are supported:
    * - fn:SinceVersion(x.y,zzz): keep/strip if version &gt;= x.y else remove
    * - fn:BeforeVersion(x.y,zzz): keep/strip if x.y &lt; version else remove
    * - fn:Deprecated(x.y,zzz): strip if version &lt; x.y else keep as-is with predicate
    * - fn:IsPDFVersion(x.y,zzz): keep/strip if version == x.y else remove
    *
    * @param str     the atomic element from an Arlington TSV file
    * @param version the PDF version being targeted. 1.0 to 2.0 inclusive. 
    * 
    * @return the version-reduced equivalent appropriate for the version
    */
   private static String reduceAtomicForVersion(String str, double version) {
        if ((str.isBlank()) || (!str.contains("fn:"))) {
            return str;
        }

        String   tsv_ver;
        String   tsv_s = str;
        // +3 = length of PDF version string "x.y"
        // +1 = trailing COMMA as predicate operand separator before atomic element
        if (str.startsWith("fn:SinceVersion(")) {
            tsv_ver = str.substring(16, 16+3);
            if (version >= Double.parseDouble(tsv_ver)) {
                tsv_s = str.substring(16+3+1, str.length()-1);
            }
            else {
                tsv_s = "";
            }
        }
        else if (str.startsWith("fn:BeforeVersion(")) {
            tsv_ver = str.substring(17, 17+3);
            if (Double.parseDouble(tsv_ver) < version) {
                tsv_s = str.substring(17+3+1, str.length()-1);
            }
            else {
                tsv_s = str;
            }
        }
        else if (str.startsWith("fn:Deprecated(")) {
            tsv_ver = str.substring(14, 14+3);
            if (version < Double.parseDouble(tsv_ver)) {
                tsv_s = str.substring(14+3+1, str.length()-1);;
            }
            else {
                tsv_s = str;
            }
        }
        else if (str.startsWith("fn:IsPDFVersion(")) {
            tsv_ver = str.substring(16, 16+3);
            if (version == Double.parseDouble(tsv_ver)) {
                tsv_s = str.substring(16+3+1, str.length()-1);
            }
            else {
                tsv_s = "";
            }
        }
        return tsv_s;
    }
    
    /**
    * Processes an Arlington Type field that might contain version
    * predicates and reduces it appropriately for the specified PDF version.
    *
    * @param str     the Type field from an Arlington TSV file
    * @param version the PDF version being targeted. 1.0 to 2.0 inclusive. 
    *
    * @return the version-reduced equivalent appropriate for the version
    */
    private static String reduceTypesForVersion(String str, double version) {
        if ((str.isBlank()) || (!str.contains("fn:"))) {
            return str;
        }

        String[] arr = str.split(";"); // split complex type
        
        String   out_types = "";
        String   tsv_s;
        for (String a : arr) {
            tsv_s = reduceAtomicForVersion(a, version);

            // Append to output if there was anything to keep
            if (!tsv_s.isBlank()) {
                if (out_types.isBlank()) {
                    out_types = tsv_s;
                } else {
                    out_types = out_types + ";" + tsv_s;
                }
            }
        }
        return out_types;
    }
    
    /**
    * Processes any complex Arlington field ([];[];[]) that might contain version
    * predicates and reduces it appropriately for the specified PDF version.
    *
    * @param str     the Link field from an Arlington TSV file
    * @param version the PDF version being targeted. 1.0 to 2.0 inclusive. 
    * 
    * @return the version-reduced equivalent appropriate for the version
    */
    private static String reduceComplexForVersion(String str, double version) {
        if ((str.isBlank()) || (!str.contains("fn:"))) {
            return str;
        }

        String[] arr = str.split(";"); // split complex type
        
        String   out_links = "";
        for (String a : arr) {
            // strip [ and ]    
            a = a.substring(1, a.length() - 1);
            
            String link = "";
            // COMMAs are ambiguous: separators or inside predicates?
            while (!a.isBlank()) {
                if (a.startsWith("fn:")) {
                    // get up to closing bracket )
                    int i = IndexOfOuterCloseBracket(a);
                    assert i != -1: "No ')' for predicate!";
                    
                    // Get encapsulating predicate incl. close bracket
                    String s = a.substring(0, i + 1);

                    if (i + 2 < a.length()) {
                        a = a.substring(i + 2, a.length());
                    }
                    else {
                        a = "";
                    }
                    String reduced = reduceAtomicForVersion(s, version);

                    // Append to output if there was anything to keep
                    if (!reduced.isBlank()) {
                        if (link.isBlank()) {
                            link = reduced;
                        } else {
                            link = link + "," + reduced;
                        }
                    }
                    // remove COMMA if not at end of string
                    if ((!a.isBlank()) && (a.charAt(0) == ',')) {
                        a = a.substring(1, a.length());
                    }
                }
                else {
                    String s = a;
                    if (a.indexOf(',') >= 0) {
                        s = a.substring(0, a.indexOf(','));
                        a = a.substring(a.indexOf(',') + 1, a.length());
                    }
                    else {
                        a = "";
                    }
                    if (link.isBlank()) {
                        link = s;
                    } else {
                        link = link + "," + s;
                    }                    
                }
            } // while
            
            if (out_links.isBlank()) {
                out_links = "[" + link + "]";
            }
            else {
                out_links = out_links + ";[" + link + "]"; 
            }
        } // for
        return out_links;
    }
    
    
    /**
    * Processes an Arlington "Required" field that might contain version
    * predicates and reduces it appropriately for the specified PDF version.
    * Outer predicate is always "fn:IsRequired(...)". Examples include:
    * - fn:IsRequired(fn:SinceVersion(1.5))
    * - fn:IsRequired(fn:BeforeVersion(1.3) || fn:IsPresent(SomeKey))
    * - fn:IsRequired(fn:SinceVersion(2.0) || fn:AnotherPredicate(...))
    *
    * @param str     the Required field from an Arlington TSV file
    * @param version the PDF version being targeted. 1.0 to 2.0 inclusive. 
    * 
    * @return the version-reduced equivalent appropriate for the version
    */
    private static String ReduceRequiredForVersion(String reqd, double version) {
        String r = reqd.substring(14, reqd.length()-1);
        String tsv_ver;
        
        if (r.startsWith("fn:BeforeVersion(")) {
            tsv_ver = r.substring(17, 17+3);
            // Don't process complex expressions (e.g. with " && " or " || ")
            if ((version < Double.parseDouble(tsv_ver)) && (reqd.indexOf(' ') == -1)) {
                return "TRUE";
            }
        }                         
        else if (r.startsWith("fn:IsPDFVersion(")) {
            tsv_ver = r.substring(16, 16+3);
            // Don't process complex expressions (e.g. with " && " or " || ")
            if ((Double.parseDouble(tsv_ver) == version) && (reqd.indexOf(' ') == -1)) {
                return "TRUE";
            }
        }
        else if (r.startsWith("fn:SinceVersion(")) {
            tsv_ver = r.substring(16, 16+3);
            if (version >= Double.parseDouble(tsv_ver)) {
                if ((reqd.indexOf(' ') == -1) || (reqd.contains(" || "))) {
                    return "TRUE";
                }
            }
            else {
                r = reqd.replace("fn:SinceVersion("+tsv_ver+") || ", "");
                return r;
            }
        }
        return reqd;
    }
     
}
