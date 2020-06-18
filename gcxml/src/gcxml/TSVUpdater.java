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
 * Contributors: Roman Toda, Frantisek Forgac, Normex
 */
package gcxml;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author fero
 */
public class TSVUpdater {
    final double[] pdf_version = {1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 2.0};
    
    public TSVUpdater(){
        for(double x : pdf_version){
            createTSV(x);
        }
    }
    
    private static void createTSV(double version){
        final String delimiter = "\t";
        final String path_to_tsv_files = System.getProperty("user.dir") + "/tsv/latest/";
        
        File[] list_of_files = null;
        File folder = new File(path_to_tsv_files);
        list_of_files = folder.listFiles();
        
        for(File file : list_of_files){
            if(file.isFile() && file.canRead() && file.exists()){
                try {
                    BufferedReader tsv_reader;
                    String temp = path_to_tsv_files;
                    String file_name = file.getName().substring(0, file.getName().length()-4);
                    temp += file_name + ".tsv";
                    tsv_reader = new BufferedReader(new FileReader(temp));
                    String[] row = null;
                    String output_string = "";
                    String entry = "";
                    //System.out.println("Processing " +file_name);
                    String current_line = tsv_reader.readLine();
                    if (current_line != null) {
                        output_string = current_line + "\n";
                    }
                    while ((current_line = tsv_reader.readLine()) != null) {
                        row = current_line.split(delimiter);
                        String temp2 = row[2];
                        if(Double.parseDouble(temp2) <= version){
                            entry += current_line + "\n";
                        }
                    }
                    if(!entry.isEmpty()){
                        output_string += entry;
                        WriteToFile(output_string, file_name, version);
                    }
                } catch (FileNotFoundException ex) {
                    Logger.getLogger(TSVUpdater.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(TSVUpdater.class.getName()).log(Level.SEVERE, null, ex);
                }
                
            }
        }
    }

    private static void WriteToFile(String output_string, String file, double version) {
        BufferedWriter out = null;
        
        String path = "";
        path +=  System.getProperty("user.dir") + "/tsv/";
        path += version + "/";
        path += file +".tsv";

        try {
            FileWriter fstream = new FileWriter(path, false);
            out = new BufferedWriter(fstream);
            out.write(output_string);
        }
        catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
        }
        finally {
            if(out != null) {
                try {
                    out.close();
                } catch (IOException ex) {
                    Logger.getLogger(TSVUpdater.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }
}
